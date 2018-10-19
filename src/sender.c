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

int send_data(int sfd, char* filename, int optionf)
{
	/* TEST
	pkt_t* pkt=pkt_new();
	pkt_set_type(pkt,1);
	pkt_set_seqnum(pkt,0);
	pkt_set_payload(pkt,"Hello world",sizeof("Hello world"));
	pkt_set_length(pkt,sizeof("Hello world"));
	send_pkt(sfd,pkt);
	printf("\n[SENT] : %s\n\n", pkt_get_payload(pkt));
	pkt_del(pkt); //à enlever
	*/

    int ret=-1;
		int fd;
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
		int sseqnum=0;
    char bufsender[512];
    memset(bufsender,0,512);
    while(1)
    {
        struct pollfd fds[2];

        fds[0].fd=fd;
        fds[0].events=POLLIN;
        fds[1].fd = sfd;
        fds[1].events = POLLIN;
        ret = poll(fds, 2, -1 );
        if (ret<0) {
            fprintf(stderr,"select error\n");
            fprintf(stderr,"ERROR: %s\n", strerror(errno));
						if(fd!=0)
						close(fd);
            return -1;
        }
        if (fds[0].revents & POLLIN)
        {
            int length=read(fd,bufsender,512);
            if(length==0)
            {
								if(fd!=0)
								close(fd);
                //printf("Fin du programme!\n");
                return 0;
            }
						//GERER SEQNUM ET WINDOW DU SR
						uint8_t seqnum=0;
						uint8_t window=1;
						pkt_t* pkt=pkt_initialize(bufsender,length,seqnum,window);
						if(pkt==NULL)
						{
							fprintf(stderr,"Error initialiazing a packet\n");
							if(fd!=0)
							close(fd);
							return -1;
						}
						send_pkt(sfd,pkt);
						memset(bufsender,0,512);
            length=0;
        }

        if (fds[1].revents & POLLIN){
					pkt_t* pktrec=pkt_new();
          if(receive_pkt(sfd,pktrec)<0)
					{
						fprintf(stderr,"Error receiving ACK/NACK");
						if(fd!=0)
						close(fd);
						return -1;
					}
					//PROCESS pktrec
        }
    }
	if(fd!=0)
	close(fd);
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
  printf("Address: %s\nPort: %d\n",first_address,port);
  if(optionf)
    printf("Filename: %s\n",filename);
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
	return EXIT_SUCCESS;

}
