#include "receiver.h"

//TODO: utiliser le timestamp pour signaler la fin du programme. Le mettre à 1 et vérifier dans le receiver si timestamp > 0
// modifier le sender pour qu'en cas d'EOF, il envoie un paquet avec le timestamp à 1 au lieu de simplement EOF.

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
  int err2 = send_pkt(sfd,ack);
  err=err || err2;
  //printf("ack\n");
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
  if(send_pkt(sfd,nack)!=0)
  {
    pkt_del(nack);
    return -1;
  }
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
  int first_received=0;
  int sseqnum=0;
  pkt_t** buffer = calloc(WINDOW_LENGTH,sizeof(pkt_t*));                      //Creation de la fenetre
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
    //TODO: ENLEVER O_TRUNC
    fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU|S_IRWXO);               //Ouverture du FileDirector
    if(fd<0)
    {
      free(buffer);
      fprintf(stderr,"Error: file might not exist\n");
      return -1;
    }
  }
  char bufreceiver[1024];
  memset(bufreceiver,0,1024);
  int acks=0; //TODO :à remplacer par le selective repeat -> correspond à s'il faut renvoyer un ack
  while(1)
  {
    int i;
    for(i=0;i<WINDOW_LENGTH;i++)                                              //Regarde si un ack peut etre envoye
    {
      if(buffer[i]!=NULL)
      {
        if(pkt_get_seqnum(buffer[i])==sseqnum)
        {
          if(process_data(fd,buffer[i])!=0)
          {
            fprintf(stderr,"Error : process data\n");
            free(buffer);
            if(close(fd)<0)
            {
              fprintf(stderr,"Error : the file wasn't closed\n");
              return -1;
            }
            //return -1;
          }
          buffer[i]=NULL;
          if(send_ack(sfd,sseqnum+1)==-1)
          {
            fprintf(stderr,"Error : sending ack\n");
            free(buffer);
            if(close(fd)<0)
            {
              fprintf(stderr,"Error : the file wasn't closed\n");
              return -1;
            }
            //return -1;
          }
          sseqnum++;
          if(sseqnum>255)
          sseqnum=0;
        }
      }
    }
    //Plus de ack a envoyer
    pkt_t* pkt=pkt_new();
    int err = receive_pkt(sfd,pkt); //TODO : check crc ou tr
    fflush(stdout);
    if(err==2)                                                                //Si le paquet recu est tronque
    {
      uint8_t pkt_seqnum=pkt_get_seqnum(pkt);
      if(seqnum_in_window(sseqnum,WINDOW_LENGTH,pkt_seqnum))                  //Si il est dans la fenetre attendue
      {
        if(send_nack(sfd,pkt_get_seqnum(pkt))==-1)
        {
          fprintf(stderr,"Error : sending nack\n");
          pkt_del(pkt);
          //return -1;
        }
        pkt_del(pkt);
      }
      else
      {
        pkt_del(pkt);
      }
    }
    else if(err==1)                                                           //Si le paquet contient le EOF
    {
      if(send_ack(sfd,pkt_get_seqnum(pkt)+1)<0)
      {
        fprintf(stderr,"Error sending final ACK\n");
        return -1;
      }
      pkt_del(pkt);
      free(buffer);
      return 0;
    }
    else if(err==0)                                                           //Si il n'est pas modifie
    {
      uint8_t pkt_seqnum=pkt_get_seqnum(pkt);
      if(seqnum_in_window(sseqnum,WINDOW_LENGTH,pkt_seqnum))                  //Si il est dans la fenetre attendue
      {
        if(sseqnum==pkt_seqnum)                                               //Si c'est le premier paquet attendu
        {
          int i;
          for(i=0;i<WINDOW_LENGTH;i++)
          {
            if(buffer[i]!=NULL)
            {
              if(pkt_get_seqnum(buffer[i])==sseqnum)
              {
                pkt_del(buffer[i]);
              }
            }
          }
          if(send_ack(sfd,sseqnum+1)!=0)
          {
            fprintf(stderr,"Error : sending ack\n");

            //return -1;                                                         //<<--------Free---------->>
          }
          first_received=1;
          sseqnum++;
          if(sseqnum>255)
          sseqnum=0;
          if(process_data(fd,pkt)!=0)
          {
            fprintf(stderr,"Error : processing data\n");

            return -1;
          }
        }
        else                                                                  //Si pas le premier paquet attendu
        {
          int boolean=0;
          int i;
          for(i=0;i<WINDOW_LENGTH && !boolean;i++)
          {
            if(buffer[i]!=NULL)
            {
              if(pkt_get_seqnum(buffer[i])==pkt_seqnum)
              {
                boolean=1;
              }
            }
          }
          if(boolean)                                                           //Si deja dans le buffer
          {
            pkt_del(pkt);
            if(first_received)
            {
              if(send_ack(sfd,sseqnum)!=0)
              {
                fprintf(stderr,"Error : sending ack\n");

                //return -1;
              }
            }
          }
          else                                                                  //Si pas encore dans le buffer
          {
            int added=0;
            int i;
            for(i=0;i<WINDOW_LENGTH;i++)
            {
              if(buffer[i]==NULL)
              {
                buffer[i]=pkt;
                i=WINDOW_LENGTH;
                added=1;
              }
            }
            if(!added)                                                          //Si pas ete ajoute
            {
              pkt_del(pkt);
              fprintf(stderr,"Error : no space on buffer - pkt discarded\n"); //<<-------------------->>
            }
            else                                                                //Si ajoute
            {
              if(first_received)
              {
                if(send_ack(sfd,sseqnum)!=0)
                {
                  fprintf(stderr,"Error : sending ack\n");

                  //return -1;
                }
              }
            }
          }
        }
      }
      else //not in window TODO: renvoyer ack? ou pas?
      {
        if(first_received)
        {
          if(send_ack(sfd,sseqnum)!=0)
          {
            fprintf(stderr,"Error : sending ack\n");

            //return -1;
          }
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
