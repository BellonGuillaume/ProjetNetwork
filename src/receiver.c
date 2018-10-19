#include "receiver.h"

int receive_data(int sfd, char* filename, int optionf)
{
  /*TEST
  pkt_t* pkt=pkt_new();
  int err=receive_pkt(sfd,pkt);
  char msg[pkt_get_length(pkt)];
  if(pkt_get_payload(pkt)==NULL)
    return -1;
  memcpy(msg,pkt_get_payload(pkt),pkt_get_length(pkt));
  //printf("about to printf\n");
  printf("\n[RECEIVED] : %s\n\n",msg);
  pkt_del(pkt);*/

    int ret=-1;
    int fd;
    if(!optionf)
    {
      fd=1;
    }
    else
    {
      fd=open(filename,O_WRONLY);
      if(fd<0)
      {
        fprintf(stderr,"Error: file might not exist\n");
				return -1;
      }
    }
    char bufreceiver[1024];
    memset(bufreceiver,0,1024);
    int acks=0; //TODO :à remplacer par le selective repeat -> correspond à s'il faut renvoyer un ack
    while(1)
    {
      if(acks)
      {
        uint8_t seqnum=0;
        uint8_t window=0;
        uint8_t type=1;
        pkt_t* ack = pkt_initialize(NULL,0,seqnum,window); //TODO :mettre le tr à 1?
        pkt_set_type(ack,type);
        send_pkt(sfd,ack);
        pkt_del(ack);
      }
      pkt_t* pkt=pkt_new();
      receive_pkt(sfd,pkt); //TODO : check crc ou tr
      int length=write(fd,pkt_get_payload(pkt),pkt_get_length(pkt));
      if(length==0)
      {
        //Fin du programme
        if(fd!=1)
          close(fd);
        return 0;
      }
    }
}

int main(int argc, char* argv[])
{
  int optionf=0;
  char first_address[16]="::1";
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
  const char* err= real_address(first_address, &addr);
  if(err!=NULL)
  {
    fprintf(stderr, "Wrong hostname %s: %s\n", first_address, err);
		return EXIT_FAILURE;
  }
  //printf("about to create_socket\n");
  int sfd = create_socket(&addr, port, NULL, -1); /* Bound */
  //printf("about to wait_for_client\n");
	if (sfd > 0 && wait_for_client(sfd) < 0) { /* Connected */ //Si j'ai un socket mais qu'il y a une erreur dans wait_for_client
		fprintf(stderr,
				"Could not connect the socket after the first message.\n");
		close(sfd);
		return EXIT_FAILURE;
	}
	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}
  //printf("about to receive_data\n");
  if(receive_data(sfd, filename, optionf) < 0) {
		fprintf(stderr, "Reception error\n");
		return EXIT_FAILURE;
	}
  return EXIT_SUCCESS;
}
