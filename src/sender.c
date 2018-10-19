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
