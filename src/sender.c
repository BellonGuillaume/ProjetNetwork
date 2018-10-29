#include "sender.h"

int countData=0;

int send_data(int sfd, char* filename, int optionf)
{
	uint8_t window_length=31;
	int ret=-1;
	int fd;
	int eof_reached=0;
	int ack_received=0;
	int flag_last_ackw=0;
	int done=0;
	if(!optionf)
	{
		fd=0;
	}
	else
	{
		fd=open(filename,O_RDONLY);
		if(fd<0)
		{
			fprintf(stderr,"Error: file might not exist\n");
			return -1;
		}
	}
	uint8_t sseqnum=0;
	char bufsender[512];
	memset(bufsender,0,512);
	window_t* window = window_new(window_length);
	if(window == NULL){
		if(close(fd)<0){
			fprintf(stderr, "Error : the file wasn't closed\n");
			return EXIT_FAILURE;
		}
		return EXIT_FAILURE;
	}
	int countTypeDiscard=0;
	while(!done) //TODO faire un if "place dans la window" -> read stdin (receive ack quand même)| else -> receive ack
	{					//TODO gérer les messages de déconnection
		node_t* n_RTT = window_check_RTT(window);
		if(n_RTT == NULL)																														//Pas de RTT
		{
			//printf("NULL\n");
			struct pollfd fds[2];

			fds[0].fd=fd;
			fds[0].events=POLLIN;
			fds[1].fd = sfd;
			fds[1].events = POLLIN;
			//printf("prépoll\n");
			ret = poll(fds, 2, 1000);
			//printf("postpoll\n");
			if (ret<0) {																															//Si erreur de Select
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
			}																																					//Pas d'erreur de Select
			int i;
			for(i=0;i<window->length;i++)
			{
				if(window->buffer[i]!=NULL)
				{
					//printf("WINDOW %d : SEQ %d\n", i, window->buffer[i]->seqnum);
				}
			}
			if (fds[0].revents & POLLIN)																							//Chose a lire dans le fichier
			{
				//printf("STDIN INPUT\n");
				if(window_is_full(window))																							//Si la fenetre est pleine
				{
					pkt_t* ack = pkt_new();
					if(receive_pkt(sfd,ack)!=PKT_OK)																			//Si accuse de reception recu
					{
						//Rien à faire
					}
					ptypes_t typeAck = pkt_get_type(ack);
					//printf("type : %d\n",pkt_get_type(ack));
					if(typeAck==PTYPE_DATA)																								//Si accuse de reception de type DATA
					//if(typeAck!=PTYPE_ACK && typeAck!= PTYPE_NACK)
					{
						countTypeDiscard++;
					}
					else if(typeAck==PTYPE_ACK)																						//Si accuse de reception de type ACK
					{
						//printf("Ack recu\n");
						window_remove(window,pkt_get_seqnum(ack)-1);
					}
					else if(typeAck==PTYPE_NACK)																					//Si accuse de reception de type NACK
					{
						pkt_t* pkt=window_find(window,pkt_get_seqnum(ack));
						if(pkt!=NULL)
						{
							countData++;
							node_t* node = window_node_with_seqnum(window, pkt_get_seqnum(pkt));
							if(node == NULL)
							{
								fprintf(stderr,"Error : node not found\n");
								if(fd!=0)
								close(fd);
								window_del(window);
								return -1;
							}
							if(send_pkt(sfd,pkt)!=0)
							{
								fprintf(stderr,"Error : sending pkt\n");
								if(fd!=0)
								close(fd);
								window_del(window);
								return -1;
							}
						  gettimeofday(&(node->time_init),NULL);
						}
					}
					pkt_del(ack);
				}
				else																																		//Si la fenetre n'est pas pleine
				{
					int length=read(fd,bufsender,512);
					if(length==0)																													//Si fin de fichier
					{
						if(fd!=0)
						if(close(fd)<0)
						{
							fprintf(stderr,"Error : close\n");
							return -1;
						}
						//printf("Fin du programme!\n");
						eof_reached=1;
					}
					if(!eof_reached){
						uint8_t seqnum=sseqnum;																								//Envoi d'un pkt
						sseqnum++;
						if(sseqnum>255)
						sseqnum=0;
						pkt_t* pkt;
						pkt=pkt_initialize(bufsender,length,seqnum,window_length);
						if(pkt==NULL)
						{
							fprintf(stderr,"Error initialiazing a packet\n");
							if(fd!=0)
							close(fd);
							window_del(window);
							return -1;
						}
						countData++;
						if(send_pkt(sfd,pkt)!=0)
						{
							fprintf(stderr,"Error : sending pkt\n");
							if(fd!=0)
							close(fd);
							pkt_del(pkt);
							window_del(window);
							return -1;
						}
						//double time_initialize = (double) gettimeofday(...)/CLOCKS_PER_SEC;
						memset(bufsender,0,512);
						if(window_add(window,pkt)<0)																					//Stockage du pkt dans la fenetre
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
			if (fds[1].revents & POLLIN)																							//Choses a lire dans le socket
			{
				//printf("présent2\n");
				pkt_t* ack = pkt_new();
				if(receive_pkt(sfd,ack)!=PKT_OK)
				{
					//Rien à faire
				}
				ptypes_t typeAck = pkt_get_type(ack);
				if(typeAck==PTYPE_DATA)
				//if(typeAck!=PTYPE_ACK && typeAck!= PTYPE_NACK)
				{
					countTypeDiscard++;
				}
				else if(typeAck==PTYPE_ACK)
				{
					//printf("Ack recu\n");
					window_remove(window,pkt_get_seqnum(ack)-1);
					if(eof_reached && window->size_used==0)
					{
						//printf("last ack received\n");
						ack_received=1;
					}
					//printf("eof : %d, ack_received %d, flag_last_ackw %d\n",eof_reached,ack_received,flag_last_ackw);
					if(eof_reached && flag_last_ackw)
					{
						done=1;
						printf("done\n");
					}
					//printf("check\n");
				}
				else if(typeAck==PTYPE_NACK)
				{
					//printf("Nack recu\n");
					pkt_t* pkt=window_find(window,pkt_get_seqnum(ack));
					if(pkt!=NULL)
					{
						countData++;
						node_t* node = window_node_with_seqnum(window,pkt_get_seqnum(pkt));
						if(node == NULL)
						{
							fprintf(stderr,"Error : node not found\n");
							if(fd!=0)
							close(fd);
							window_del(window);
							return -1;
						}
						if(send_pkt(sfd,pkt)!=0)
						{
							fprintf(stderr,"Error : sending pkt\n");
							if(fd!=0)
							close(fd);
							window_del(window);
							return -1;
						}
					  gettimeofday(&(node->time_init),NULL);
					}
				}
				pkt_del(ack);
			}
			if(eof_reached && window->size_used==0)
			{
				//printf("last ack received\n");
				ack_received=1;
			}
			if((eof_reached && ack_received) && !flag_last_ackw)
			{
				uint8_t seqnum=sseqnum;																									//Envoi d'un pkt
				sseqnum++;
				if(sseqnum>255)
				sseqnum=0;
				pkt_t* end_pkt = pkt_initialize(NULL,0,seqnum,window_length);
				if(end_pkt==NULL)
				{
					fprintf(stderr, "Error : creating ending flag\n");
					if(fd!=0)
					close(fd);
					window_del(window);
					return -1;
				}
				if(send_pkt(sfd,end_pkt)!=0)
				{
					fprintf(stderr, "Error : sending ending flag\n");
					if(fd!=0)
					close(fd);
					pkt_del(end_pkt);
					window_del(window);
					return -1;
				}
				if(window_add(window,end_pkt)!=0)
				{
					fprintf(stderr, "Error : adding disconnecting pkt to the window\n");
					if(fd!=0)
					close(fd);
					pkt_del(end_pkt);
					window_del(window);
					return -1;
				}
				flag_last_ackw=1;
				countData++;
				/*A enlever pour un disconnect pas abrupt*/done=1;											//<--------------------------
			}
		}
		else 																																				//RTT atteint
		{
			//printf("Resending pkt\n");
			if(send_pkt(sfd,n_RTT->pkt)!=0)
			{
				fprintf(stderr,"Error : sending pkt\n");
				if(fd!=0)
				close(fd);
				window_del(window);
				return -1;
			}
			countData++;
		  gettimeofday(&(n_RTT->time_init),NULL);
		}
		//printf("futur\n");
	} //Fin de la boucle while
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
	char first_address[16]="";
	int port=-1;
	char filename[255]="";
	if(ValidateArgs(argc,argv,&optionf,filename,first_address,&port)!=0)
	{
		return EXIT_FAILURE;
	}
	printf("\n=== Sender ===\n");
	printf("--- Address: %s ---\n--- Port: %d ---\n",first_address,port);
	if(optionf)
	printf("--- Filename: %s ---\n",filename);
	struct sockaddr_in6 addr;
	const char *err = real_address(first_address, &addr);
	if (err!=NULL) {
		fprintf(stderr, "Wrong hostname %s: %s\n", first_address, err);
		return EXIT_FAILURE;
	}
	int sfd = create_socket(NULL,-1,&addr,port);
	if(sfd < 0) {
		fprintf(stderr, "Failed to create the socket\n");
		return EXIT_FAILURE;
	}
	if(send_data(sfd, filename, optionf) < 0) {
		fprintf(stderr, "Sending error\n");
		close(sfd);
		return EXIT_FAILURE;
	}
	printf("=== Data successfully sent ===\n");
	printf("Number of PKT_DATA sent : %d\n",countData);
	close(sfd);
	return EXIT_SUCCESS;

}
