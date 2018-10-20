#include "receiver.h"

//TODO: Checker que les seqnum et autres uint8_t ne sont pas écrits en int

int countNack=0;
int countAck=0;

int send_ack(int sfd, int seqnum)
{
  ptypes_t type=PTYPE_ACK;
  pkt_t* ack = pkt_initialize(NULL,0,seqnum,WINDOW_LENGTH); //TODO :mettre le tr à 1?
  if(ack==NULL)
  return -1;
  int err;
  err=pkt_set_type(ack,type);
  err=err && send_pkt(sfd,ack);
  printf("ack\n");
  countAck++;
  pkt_del(ack);
  return err;
}

int send_nack(int sfd, int seqnum)
{
  countNack++;
  ptypes_t type=PTYPE_NACK;
  pkt_t* nack = pkt_initialize(NULL,0,seqnum,WINDOW_LENGTH); //TODO :mettre le tr à 1?
  if(nack==NULL)
  return -1;
  int err;
  err=pkt_set_type(nack,type);
  err=err && send_pkt(sfd,nack);
  pkt_del(nack);
  return err;
}

int process_data(int fd, pkt_t* pkt)
{
  int length=write(fd,pkt_get_payload(pkt),pkt_get_length(pkt));
  pkt_del(pkt);
  if(length==0)
  {
    //Fin du programme
    if(fd!=1)
      close(fd);
    return 0;
  }
  return 0;
}

int receive_data(int sfd, char* filename, int optionf)
{
    int sseqnum=0;
    pkt_t** buffer = calloc(WINDOW_LENGTH,sizeof(pkt_t*));
    if(buffer==NULL)
    {
      fprintf(stderr, "Error : calloc fail\n");
      return -1;
    }
    int ret=-1;
    int fd;
    if(!optionf)
    {
      fd=1;
    }
    else
    {
      fd=open(filename,O_WRONLY|O_CREAT,S_IRWXU|S_IRWXO);
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
      for(int i=0;i<WINDOW_LENGTH;i++)
      {
        if(buffer[i]!=NULL)
        {
          if(pkt_get_seqnum(buffer[i])==sseqnum)
          {
            if(process_data(fd,buffer[i])!=0)
            {
              fprintf(stderr,"Error : process data\n");
              return -1;
            }
            if(send_ack(sfd,sseqnum)==-1)
            {
              fprintf(stderr,"Error : sending ack\n");
              return -1;
            }
            sseqnum++;
          }
        }
      }

      pkt_t* pkt=pkt_new();
      int err = receive_pkt(sfd,pkt); //TODO : check crc ou tr
      fflush(stdout);
      if(err==2)
      {
        uint8_t pkt_seqnum=pkt_get_seqnum(pkt);
        if(seqnum_in_window(sseqnum,WINDOW_LENGTH,pkt_seqnum))
        {
          if(send_nack(sfd,pkt_get_seqnum(pkt))==-1)
          {
            fprintf(stderr,"Error : sending nack\n");

            return -1;
          }
          pkt_del(pkt);
        }
        else
        {
          pkt_del(pkt);
        }
      }
      else if(err==1)
      {
        //printf("Fin du receiver\n");
        free(buffer);
        return 0;
      }
      else if(err==0)
      {
        uint8_t pkt_seqnum=pkt_get_seqnum(pkt);
        if(seqnum_in_window(sseqnum,WINDOW_LENGTH,pkt_seqnum))
        {
          if(sseqnum==pkt_seqnum)
          {
            for(int i=0;i<WINDOW_LENGTH;i++)
            {
              if(buffer[i]!=NULL)
              {
                if(pkt_get_seqnum(buffer[i])==sseqnum)
                {
                  pkt_del(buffer[i]);
                }
              }
            }
            if(send_ack(sfd,sseqnum)!=0)
            {
              fprintf(stderr,"Error : sending ack\n");

              return -1;
            }
            sseqnum++;
            if(process_data(fd,pkt)!=0)
            {
              fprintf(stderr,"Error : processing data\n");

              return -1;
            }
          }
          else
          {
            int boolean=0;
            for(int i=0;i<WINDOW_LENGTH;i++)
            {
              if(buffer[i]!=NULL)
              {
                if(pkt_get_seqnum(buffer[i])==pkt_seqnum)
                {
                  boolean=1;
                }
              }
            }
            if(boolean)
            {
              pkt_del(pkt);
              if(send_ack(sfd,sseqnum-1)!=0)
              {
                fprintf(stderr,"Error : sending ack\n");

                return -1;
              }
            }
            else
            {
              int added=0;
              for(int i=0;i<WINDOW_LENGTH;i++)
              {
                if(buffer[i]==NULL)
                {
                  buffer[i]=pkt;
                  i=WINDOW_LENGTH;
                  added=1;
                }
              }
              if(!added)
              {
                pkt_del(pkt);
                fprintf(stderr,"Error : no space on buffer - pkt discarded\n");
              }
              else
              {
                if(send_ack(sfd,pkt_seqnum-1)!=0)
                {
                  fprintf(stderr,"Error : sending ack\n");

                  return -1;
                }
              }
            }
          }
        }
        else //not in window TODO: renvoyer ack? ou pas?
        {
          if(send_ack(sfd,sseqnum-1)!=0)
          {
            fprintf(stderr,"Error : sending ack\n");

            return -1;
          }
          pkt_del(pkt);
        }
      }
      //end of loop
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
  printf("\n=== Receiver ===\n");
  printf("--- Address: %s ---\n--- Port: %d ---\n",first_address,port);
  if(optionf)
  printf("--- Filename: %s ---\n",filename);
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
  printf("=== Data successfully received ===\n");
  printf("Number of NACK : %d\n",countNack);
  printf("Number of ACK : %d\n",countAck);
  return EXIT_SUCCESS;
}
