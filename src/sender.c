#include "sender.h"

//Rappel de la structure
/*struct __attribute__((__packed_)) pkt {
uint8_t window:5;
uint8_t tr:1;
uint8_t type:2;
uint8_t seqnum;
uint16_t length;
uint32_t timestamp;
uint32_t crc1;
char* payload;
uint32_t crc2;
};*/

int countData=0;

int send_data(int sfd, char* filename, int optionf)
{
	uint8_t window_length=4;
	int ret=-1;
	int fd;
	int eof_reached=0;
	int ack_received=0;
	clock_t RTT = 4;
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
	while(!(eof_reached && ack_received)) //TODO faire un if "place dans la window" -> read stdin (receive ack quand même)| else -> receive ack
	{					//TODO gérer les messages de déconnection
		node_t* n_RTT = window_check_RTT(window);
		if(n_RTT == NULL)
		{
			//printf("NULL\n");
			struct pollfd fds[2];

			fds[0].fd=fd;
			fds[0].events=POLLIN;
			fds[1].fd = sfd;
			fds[1].events = POLLIN;
			//printf("prépoll\n");
			ret = poll(fds, 2, 20);
			//printf("postpoll\n");
			if (ret<0) {
				fprintf(stderr,"select error\n");
				fprintf(stderr,"ERROR: %s\n", strerror(errno));
				if(fd!=0)
				{
					if(close(fd)<0)
					{
						fprintf(stder,"Error : the file wasn't closed\n");
						window_del(window);
						return -1;
					}
				}
				window_del(window);
				return -1;
			}
			//printf("passé\n");
			if (fds[0].revents & POLLIN)
			{
				//printf("STDIN INPUT\n");
				if(window_is_full(window))
				{
					pkt_t* ack = pkt_new();
					if(receive_pkt(sfd,ack)!=PKT_OK)
					{
						fprintf(stderr,"Error receiving ACK/NACK\n");//TODO
					}
					ptypes_t typeAck = pkt_get_type(ack);
					//printf("type : %d\n",pkt_get_type(ack));
					if(typeAck==PTYPE_DATA)
					//if(typeAck!=PTYPE_ACK && typeAck!= PTYPE_NACK)
					{
						countTypeDiscard++;
					}
					else if(typeAck==PTYPE_ACK)
					{
						//printf("Ack recu\n");
						window_remove(window,pkt_get_seqnum(ack));
					}
					else if(typeAck==PTYPE_NACK)
					{
						pkt_t* pkt=window_find(window,pkt_get_seqnum(ack));
						if(pkt!=NULL)
						{
							countData++;
							if(send_pkt(sfd,pkt)!=0)
							{
								fprintf(stderr,"Error : sending pkt\n");
								pkt_del(pkt);
								pkt_del(ack);
								window_del(window);
								return -1;
							}
						}
					}
					pkt_del(ack);
				}
				else
				{
					int length=read(fd,bufsender,512);
					if(length==0)
					{
						if(fd!=0)
						if(close(fd)<0)
						{
							fprintf(stderr,"Error : close\n");
							window_del(window);
							return -1;
						}
						//printf("Fin du programme!\n");
						eof_reached=1;
					}
					//GERER SEQNUM ET WINDOW DU SR
					uint8_t seqnum=sseqnum;
					sseqnum++;
					if(sseqnum>255)
					{
						sseqnum = 0;
					}
					pkt_t* pkt=pkt_initialize(bufsender,length,seqnum,window_length);
					if(pkt==NULL)
					{
						fprintf(stderr,"Error initialiazing a packet\n");
						if(close(fd)<0)
						{
							fprintf(stderr,"Error : close\n");
							window_del(window);
							return -1;
						}
						window_del(window);
						return -1;
					}
					countData++;
					if(send_pkt(sfd,pkt)!=0)
					{
						fprintf(stderr,"Error : sending pkt\n");
						if(close(fd)<0)
						{
							fprintf(stderr,"Error : close\n");
							pkt_del(pkt);
							window_del(window);
							return -1;
						}
						pkt_del(pkt);
						window_del(window);
						return -1;
					}
					memset(bufsender,0,512);
					if(window_add(window,pkt)<0)
					{
						fprintf(stderr, "Error : pkt not added on the window\n");
						if(close(fd)<0)
						{
							fprintf(stderr,"Error : close\n");
							pkt_del(pkt);
							window_del(window);
							return -1;
						}
						pkt_del(pkt);
						window_del(window);
						return -1;
					}
					pkt_del(pkt);
				}
			}
			if (fds[1].revents & POLLIN) //TODO : double poll
			{
				//printf("présent2\n");
				pkt_t* ack = pkt_new();
				if(receive_pkt(sfd,ack)!=PKT_OK)
				{
					fprintf(stderr,"Error receiving ACK/NACK\n");//TODO pkt_del(ack);
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
					window_remove(window,pkt_get_seqnum(ack));
					if(eof_reached && window->size_used==0)
					{
						ack_received=1;
					}
				}
				else if(typeAck==PTYPE_NACK)
				{
					pkt_t* pkt=window_find(window,pkt_get_seqnum(ack));
					if(pkt!=NULL)
					{
						countData++;
						if(send_pkt(sfd,pkt)!=0)
						{
							fprintf(stderr,"Error : sending pkt\n");
							if(close(fd)<0)
							{
								fprintf(stderr,"Error : close\n");
								pkt_del(pkt);
								pkt_del(ack);
								window_del(window);
								return -1;
							}
							pkt_del(pkt);
							pkt_del(ack);
							window_del(window);
							return -1;
						}
					}
				}
				pkt_del(ack);
			}
			if(eof_reached && ack_received)
			{
				if(write(sfd,"EOF",sizeof("EOF"))<0)
				{
					fprintf(stderr, "Error : sending ending flag\n");
					if(close(fd)<0)
					{
						fprintf(stderr,"Error : close\n");
						window_del(window);
						return -1;
					}
					window_del(window);
					return -1;
				}
			}
		}
		else //RTT atteint
		{
			//printf("Resending pkt\n");
			if(send_pkt(sfd,n_RTT->pkt)!=0)
			{
				fprintf(stderr,"Error : sending pkt\n");
				if(close(fd)<0)
				{
					fprintf(stderr,"Error : close\n");
					window_del(window);
					return -1;
				}
				window_del(window);
				return -1;
			}
			countData++;
			n_RTT->time = clock();
		}
		//printf("futur\n");
	} //Fin de la boucle while
	window_del(window);
	if(fd!=0)
	{
		if(close(fd)<0)
		{
			fprintf(stderr,"Error : close\n");
			return -1;
		}
		return -1;
	}
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
		return EXIT_FAILURE;
	}
	printf("=== Data successfully sent ===\n");
	printf("Number of Data sent : %d\n",countData);
	return EXIT_SUCCESS;

}
