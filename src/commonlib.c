#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <ctype.h>

#define WINDOW_LENGTH 31

/* Read and processes the arguments argv, assigns it to the arguments given in
 * the fuction. They are left untouched if argv doesnt respect the right format.
 * @argc: argc from the main
 * @argv: argv from the main
 * @POST: optionf: pointer towards the flag indicating if a file was given
 * @POST: filename: file given in the args
 * @POST: first_address: address given in the args
 * @POST: port: port given in the args
 * @return: 0 in case of success, -1 otherwise
 */
int ValidateArgs(int argc, char* argv[],int* optionf,char* filename,char* first_address,int* port)
{
	int tmpf=-1;
	/*printf("%d\n",argc);
	for(int i=0;i<argc;i++)
	{
		printf("%s\n",argv[i]);
	}
	printf("\n");*/
	if (argc < 3){
		fprintf(stderr, "\nUsage:\n\n./sender   [-f filename] hostname port\n./receiver [-f filename] hostname port\n\n");
		return EXIT_FAILURE;
	}
  int option = getopt(argc,argv,"f:");
  if(option==-1)
  {
    tmpf==0;
		if(argc>3)
		{
			fprintf(stderr, "\nUsage:\n\n./sender   [-f filename] hostname port\n./receiver [-f filename] hostname port\n\n");
			return EXIT_FAILURE;
		}
  }
  else
  {
		if(argc < 5)
		{
			fprintf(stderr, "\nUsage:\n\n./sender   [-f filename] hostname port\n./receiver [-f filename] hostname port\n\n");
			return EXIT_FAILURE;
		}
    switch(option)
    {
      case 'f' :
      {
        tmpf=1;
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
	if(tmpf!=1)
	{
		*optionf=0;
	}
	else
	{
		*optionf=1;
	}
  memcpy(first_address,argv[argc-2],strlen(argv[argc-2]));
	*port = atoi(argv[argc-1]);
  return 0;
}

int seqnum_in_window(uint8_t sseqnum, uint8_t window_length, uint8_t pkt_seqnum)
{
	if(pkt_seqnum<sseqnum || pkt_seqnum>=(sseqnum+window_length))
		return 0;
	return 1;
}
