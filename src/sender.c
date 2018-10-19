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

int ValidateArgs(int argc, char* argv[],int* optionf,char* filename,char* first_address,int* port)
{
	if (argc < 3){
		fprintf(stderr, "Pas assez d'arguments\n");
		return EXIT_FAILURE;
	}
  int option = getopt(argc,argv,"f:");
  if(option==-1)
  {
    *optionf==0;
  }
  else
  {
    switch(option)
    {
      case 'f' :
      {
        *optionf=1;
        memcpy(filename,optarg,strlen(optarg));
        break;
      }
      default :
      {
        fprintf(stderr, "Erreur option\n");
        return EXIT_FAILURE;
      }
    }
  }
  memcpy(first_address,argv[argc-2],strlen(argv[argc-2]));
	*port = atoi(argv[argc-1]);
  return 0;
}

int send_socket(int sfd, char* filename, int optionf){
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
  printf("1st address: %s\nport: %d\n",first_address,port);
  if(optionf)
    printf("filename: %s\n",filename);
	struct sockaddr_in6 addr;
	const char *err = real_address(first_address, &addr);
	if (err!=NULL) {
		fprintf(stderr, "Probleme avec l'hostname %s: %s\n", first_address, err);
		return EXIT_FAILURE;
	}
	int sfd = create_socket(NULL,0,&addr,port);
	if(sfd < 0) {
		fprintf(stderr, "Erreur dans la création du socket\n");
		return EXIT_FAILURE;
	}
	if(send_socket(sfd, filename, optionf) < 0) {
		fprintf(stderr, "Erreur dans l'envoi\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;

}

//Avec l'aide de Louis Colin

/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval)
{
    struct addrinfo hints;
    struct addrinfo* res;
    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags=AI_CANONNAME;

    if(getaddrinfo(address, NULL, &hints, &res)!=0)
    {
        //fprintf(stderr,"getaddrinfo fail\n");
        return "Address not found\n";
    }

    struct sockaddr_in6* tmp = (struct sockaddr_in6*) res->ai_addr;
    memcpy(rval,tmp,sizeof(*tmp));
    if(sizeof(rval)<0)
    return "Erreur return\n";

    freeaddrinfo(res);

    return NULL;
}

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send_socket data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){

    //fprintf(stderr,"===CREATION SOCKET===\n");
    int sfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if(sfd==-1)
    {
        fprintf(stderr,"Erreur création socket\n");
        return -1;
    }
    //fprintf(stderr,"===CREATION SOCKET OK===\n");

    size_t addrlen = sizeof(struct sockaddr_in6);

    if(src_port>0)
    {
        source_addr->sin6_port=htons(src_port);
        //fprintf(stderr,"initialisation source OK\n");
    }


    if(dst_port>0)
    {
        dest_addr->sin6_port=htons(dst_port);
        //fprintf(stderr,"initialisation dest OK\n");
    }

    if(source_addr!=NULL)
    {
        if(bind(sfd,(struct sockaddr*) source_addr, addrlen)!=0)
        {
            fprintf(stderr,"error bind\n");
            return -1;
        }
        //fprintf(stderr,"bind OK\n");
    }

    if(dest_addr!=NULL)
    {
        if(connect(sfd,(struct sockaddr*) dest_addr, addrlen)!=0)
        {
            fprintf(stderr,"error connect\n");
            return -1;
        }
        //fprintf(stderr,"connect OK\n");
    }

    return sfd;
}
