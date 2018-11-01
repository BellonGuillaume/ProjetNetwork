#include "sender.h"

int countData=0;
uint8_t end_seqnum = -1;
struct timeval end_init;

int send_data(int sfd, char* filename, int optionf)
{
	uint8_t window_length=1;
	int ret=-1;
	int fd;
	int eof_reached=0;
	int ack_received=0;
	int flag_last_ackw=0;
	int done=0;
	if(!optionf)
	{
		fd=0;																																				//Initialisation du fd à stdin (si pas de fichier à lire)
	}
	else
	{
		fd=open(filename,O_RDONLY);																									//Ouverture du fichier (si fichier à lire)
		if(fd<0)
		{
			fprintf(stderr,"Error: file might not exist\n");
			return -1;
		}
	}
	uint8_t sseqnum=0;
	char bufsender[512];
	memset(bufsender,0,512);
	window_t* window = window_new(31);																						//Création de la fenêtre
	if(window == NULL){
		if(close(fd)<0){
			fprintf(stderr, "Error : the file wasn't closed\n");
			return EXIT_FAILURE;
		}
		return EXIT_FAILURE;
	}
	int countTypeDiscard=0;
	while(!done)  																																//Début de la boucle d'envoi de DATA et de réception de ACK/NACK
	{
		node_t* n_RTT = window_check_RTT(window);
		if(n_RTT == NULL)																														//Si aucun des RTT de la fenêtre n'a timeout
		{
			struct pollfd fds[2];

			fds[0].fd=fd;
			fds[0].events=POLLIN;
			fds[1].fd = sfd;
			fds[1].events = POLLIN;
			ret = poll(fds, 2, 1000);
			if (ret<0) {																																//Si erreur de Select
				fprintf(stderr,"select error\n");
				fprintf(stderr,"ERROR: %s\n", strerror(errno));
				if(fd!=0)
				{
					if(close(fd)<0)
					{
						fprintf(stderr,"Error : the file wasn't closed\n");
						window_del(window);
						return -1;
					}
				}
				window_del(window);
				return -1;
			}
			if (fds[0].revents & POLLIN)																								//S'il y a des choses à lire dans le fichier
			{
				if(window_is_full(window, window_length))																		//Si la fenetre est pleine
				{
					pkt_t* ack = pkt_new();
					if(receive_pkt(sfd,ack)!=PKT_OK)																						//Réception d'un packet (non-blocant)
					{
						//Rien à faire																														//Si rien n'a été reçu
					}
					ptypes_t typeAck = pkt_get_type(ack);
					if(typeAck==PTYPE_DATA)																											//Si pkt reçu de type DATA
					{
						countTypeDiscard++;																													//Discarded
					}
					else if(typeAck==PTYPE_ACK)																									//Si pkt reçu de type ACK
					{
						window_length = pkt_get_window(ack);
						window_remove(window,pkt_get_seqnum(ack)-1);																//On peut enlever les pkt en attente dans la window qui ont été acknowledged
					}
					else if(typeAck==PTYPE_NACK)																								//Si pkt reçu de type NACK
					{
						window_length = pkt_get_window(ack);
						pkt_t* pkt=window_find(window,pkt_get_seqnum(ack));
						if(pkt!=NULL)																																//Si pkt de ce numéro de séquence fait partie de la window
						{
							countData++;
							node_t* node = window_node_with_seqnum(window, pkt_get_seqnum(pkt));				//On récupère ce pkt
							if(node == NULL)
							{
								fprintf(stderr,"Error : node not found\n");
								if(fd!=0)
								close(fd);
								window_del(window);
								return -1;
							}
							if(send_pkt(sfd,pkt)!=0)																										//On le renvoiee
							{
								fprintf(stderr,"Error : sending pkt\n");
								if(fd!=0)
								close(fd);
								window_del(window);
								return -1;
							}
						  gettimeofday(&(node->time_init),NULL);																			//On reset son RTT
						}
					}
					pkt_del(ack);
				}
				else																																				//Si la fenetre n'est pas pleine
				{
					int length=read(fd,bufsender,512);																					//On lit stdin ou le fichier
					if(length==0)																																//Si fin de fichier
					{
						if(fd!=0)
						if(close(fd)<0)
						{
							fprintf(stderr,"Error : close\n");
							return -1;
						}
						eof_reached=1;																															//Flag de fin de fichier 0->1
					}
					if(!eof_reached){																														//Si l'on n'a pas atteint la fin du fichier
						uint8_t seqnum=sseqnum;
						sseqnum++;
						if(sseqnum>255)
						sseqnum=0;
						pkt_t* pkt;
						pkt=pkt_initialize(bufsender,length,seqnum,0);															//On initialise un pkt
						if(pkt==NULL)
						{
							fprintf(stderr,"Error initialiazing a packet\n");
							if(fd!=0)
							close(fd);
							window_del(window);
							return -1;
						}
						countData++;
						if(send_pkt(sfd,pkt)!=0)																										//On envoie ce  pkt
						{
							fprintf(stderr,"Error : sending pkt\n");
							if(fd!=0)
							close(fd);
							pkt_del(pkt);
							window_del(window);
							return -1;
						}
						memset(bufsender,0,512);
						if(window_add(window,pkt)<0)																								//On le stocke dans la fenetre
						{
							fprintf(stderr, "Error : pkt not added on the window\n");
							window_del(window);
							pkt_del(pkt);
							if(close(fd)<0){
								fprintf(stderr, "Error : the file wasn't closed\n");
								return EXIT_FAILURE;
							}
							return EXIT_FAILURE;
						}
					}
				}
			}
			if (fds[1].revents & POLLIN)																								//S'il y a des choses à lire dans le socket
			{
				pkt_t* ack = pkt_new();
				if(receive_pkt(sfd,ack)!=PKT_OK)																						//Réception d'un pkt (non-blocant)
				{
					//Rien à faire
				}
				ptypes_t typeAck = pkt_get_type(ack);
				if(typeAck==PTYPE_DATA)																											//Si c'est pkt de type DATA
				{
					countTypeDiscard++;																													//Discarded
				}
				else if(typeAck==PTYPE_ACK)																									//Si c'est un pkt de type ACK
				{
					window_length = pkt_get_window(ack);
					window_remove(window,pkt_get_seqnum(ack)-1);																//On retire le pkt du même seqnum de la window
					if(eof_reached && window->size_used==0)																			//Si c'est le dernier ACK (que l'on a atteint la fin du fichier et que tous les pkt en attente ont été envoyés)
					{
						ack_received=1;																															//Flag de tout envoyé et reçu 0->1
					}
					if(eof_reached && flag_last_ackw)																						//Si l'on a reçu l'ACK du message de déconnection
					{
						done=1;																																			//On a terminé (on rempli la condition d'arrêt de la boucle)
					}
				}
				else if(typeAck==PTYPE_NACK)																								//Si c'est un pkt de type NACK
				{
					window_length = pkt_get_window(ack);
					pkt_t* pkt=window_find(window,pkt_get_seqnum(ack));													//On regarde s'il est dans la window
					if(pkt!=NULL)																																//S'il y est
					{
						countData++;
						node_t* node = window_node_with_seqnum(window,pkt_get_seqnum(pkt));					//On retrouve le pkt de même seqnum
						if(node == NULL)
						{
							fprintf(stderr,"Error : node not found\n");
							if(fd!=0)
							close(fd);
							window_del(window);
							return -1;
						}
						if(send_pkt(sfd,pkt)!=0)																										//On le renvoie
						{
							fprintf(stderr,"Error : sending pkt\n");
							if(fd!=0)
							close(fd);
							window_del(window);
							return -1;
						}
					  gettimeofday(&(node->time_init),NULL);																			//On reset son RTT
					}
				}
				pkt_del(ack);
			}
			if(eof_reached && window->size_used==0)																			//Si l'on a atteint la fin du fichier et qu'ils n'y a plus de pkt en attente d'être envoyé
			{
				ack_received=1;																															//Flag toutes les données envoyées et réçues 0->1
			}
			if((eof_reached && ack_received) && !flag_last_ackw)												//Si l'on a atteint la fin du fichier et que toutes les données ont été envoyées et reçues (et que l'on a pas encore envoyé la demande de disconnect)
			{
				uint8_t seqnum=sseqnum;
				sseqnum++;
				if(sseqnum>255)
				sseqnum=0;
				end_seqnum = seqnum;
				pkt_t* end_pkt = pkt_initialize(NULL,0,seqnum,0);														//On initialise un pkt de taille 0 et de payload null (pour signaler la déconnection)
				if(end_pkt==NULL)
				{
					fprintf(stderr, "Error : creating ending flag\n");
					if(fd!=0)
					close(fd);
					window_del(window);
					return -1;
				}
				if(send_pkt(sfd,end_pkt)!=0)																								//On envoi ce pkt
				{
					fprintf(stderr, "Error : sending ending flag\n");
					if(fd!=0)
					close(fd);
					pkt_del(end_pkt);
					window_del(window);
					return -1;
				}
				gettimeofday(&end_init,NULL);																								//On set un timer (pour le temps d'attente de réponse max)
				if(window_add(window,end_pkt)!=0)																						//On ajoute ce pkt à la window
				{
					fprintf(stderr, "Error : adding disconnecting pkt to the window\n");
					if(fd!=0)
					close(fd);
					pkt_del(end_pkt);
					window_del(window);
					return -1;
				}
				flag_last_ackw=1;																														//Flag de demande de déconnection envoyée 0->1
				countData++;
				/*A ajouter pour un disconnect abrupt*///done=1;														//Si l'on veut un disconnect abrupt, on rempli la condition d'arrêt de la boucle
			}
		}
		else 																																				//Si l'un des pkt de la window a son RTT timeout
		{
			if(pkt_get_seqnum(n_RTT->pkt) != end_seqnum)																//Si ce pkt n'est pas la demande de déconnection
			{
				//printf("Resending pkt\n");
				if(send_pkt(sfd,n_RTT->pkt)!=0)																							//On le renvoie
				{
					fprintf(stderr,"Error : sending pkt\n");
					if(fd!=0)
					close(fd);
					window_del(window);
					return -1;
				}
				countData++;
				gettimeofday(&(n_RTT->time_init),NULL);																			//On reset son RTT
			}
			else																																				//Si c'est la demande de déconnection
			{
				struct timeval disconnect;
				gettimeofday(&disconnect,NULL);
				if(disconnect.tv_sec - end_init.tv_sec > 10)																//Si le timer d'attente max à timeout
				{
					if(fd!=0)
					close(fd);
					window_del(window);
					return 0;																																		//Fin de l'attente, déconnection abrupte
				}
				else																																				//Si le timer n'a pas timout
				{
					if(send_pkt(sfd,n_RTT->pkt)!=0)																							//On renvoie la demande de déconnection
					{
						if(fd!=0)
						close(fd);
						window_del(window);
						return 0;
					}
					countData++;
					gettimeofday(&(n_RTT->time_init),NULL);																			//On reset son RTT
				}
			}
		}
	} 																																						//Fin de la boucle while (free et close)
	window_del(window);
	if(fd!=0)
	close(fd);
	if(countTypeDiscard!=0)
	{
		fprintf(stderr,"PTYPE_DATA pkt discarded by sender : %d \n",countTypeDiscard);
	}
	return 0;
}

int main (int argc, char* argv[])
{
	int optionf = 0;
	char first_address[256]="";
	int port=-1;
	char filename[256]="";
	if(ValidateArgs(argc,argv,&optionf,filename,first_address,&port)!=0)					//Interpretation des arguments
	{
		return EXIT_FAILURE;
	}
	printf("\n=== Sender ===\n");
	printf("--- Address: %s ---\n--- Port: %d ---\n",first_address,port);
	if(optionf)
	printf("--- Filename: %s ---\n",filename);
	struct sockaddr_in6 addr;
	const char *err = real_address(first_address, &addr);													//Trouve l'adresse
	if (err!=NULL) {
		fprintf(stderr, "Wrong hostname %s: %s\n", first_address, err);
		return EXIT_FAILURE;
	}
	int sfd = create_socket(NULL,-1,&addr,port);																	//Création du socket
	if(sfd < 0) {
		fprintf(stderr, "Failed to create the socket\n");
		return EXIT_FAILURE;
	}
	if(send_data(sfd, filename, optionf) < 0) {																		//Lancement de l'envoi des données
		fprintf(stderr, "Sending error\n");
		close(sfd);
		return EXIT_FAILURE;
	}
	printf("=== Data successfully sent ===\n");
	printf("Number of PKT_DATA sent : %d\n",countData);
	close(sfd);
	return EXIT_SUCCESS;																													//Fin du programme (succès)

}
