#include "receiver.h"

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

int receive_data(int sfd, char* filename, int optionf)
{
  pkt_t* pkt=NULL;
  int err=receive_pkt(sfd,pkt);
  printf("about to pkt_get_payload\n");
  char* msg;
  memcpy(msg,pkt_get_payload(pkt),pkt_get_length(pkt));
  printf("about to printf\n");
  printf("\n[RECEIVED] : %s\n\n",msg);
  pkt_del(pkt);
  return 0;
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
  printf("about to create_socket\n");
  int sfd = create_socket(&addr, port, NULL, -1); /* Bound */
  printf("about to wait_for_client\n");
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
  printf("about to receive_data\n");
  if(receive_data(sfd, filename, optionf) < 0) {
		fprintf(stderr, "Reception error\n");
		return EXIT_FAILURE;
	}
  return EXIT_SUCCESS;
}
