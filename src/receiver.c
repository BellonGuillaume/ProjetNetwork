#include "receiver.h"

int countNack=0;
int countAck=0;
uint8_t size_used = 0;

int send_ack(int sfd, int seqnum)
{
  ptypes_t type=PTYPE_ACK;
  pkt_t* ack = pkt_initialize(NULL,0,seqnum,WINDOW_LENGTH-size_used);
  if(ack==NULL)
  return -1;
  int err;
  err=pkt_set_type(ack,type);
  int err2 = send_pkt(sfd,ack);
  err=err || err2;
  countAck++;
  pkt_del(ack);
  return err;
}

int send_nack(int sfd, int seqnum)
{
  countNack++;
  ptypes_t type=PTYPE_NACK;
  pkt_t* nack = pkt_initialize(NULL,0,seqnum,WINDOW_LENGTH-size_used);
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
  if(length==0)                                                                 //Fin du programme
  {
    if(fd!=1)
    close(fd);
    return 0;
  }
  return 0;
}

int receive_data(int sfd, char* filename, int optionf)
{
  int first_received=0;
  int sseqnum=0;                                                                //Plus petit seqnum qu'on peut recevoir
  pkt_t** buffer = calloc(WINDOW_LENGTH,sizeof(pkt_t*));                        //Creation de la fenetre de reception
  if(buffer==NULL)
  {
    fprintf(stderr, "Error : calloc fail\n");
    return -1;
  }
  int fd;
  if(!optionf)                                                                  //Si pas option f
  {
    fd=1;                                                                       //  FileDirector = Entree standard
  }
  else                                                                          //Si option f
  {
    //TODO: ENLEVER O_TRUNC
    fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU|S_IRWXO);                 //  FileDirector = Fichier donne en argument
    if(fd<0)
    {
      free(buffer);
      fprintf(stderr,"Error: file might not exist\n");
      return -1;
    }
  }
  while(1)                                                                      //Debut de la boucle d'execution
  {
    int i;
    for(i=0;i<WINDOW_LENGTH;i++)                                                //  Regarde si un ack peut etre envoye
    {
      if(buffer[i]!=NULL)
      {
        if(pkt_get_seqnum(buffer[i])==sseqnum)                                  //    Si le seqnum du pkt dans le buffer = sseqnum
        {
          if(process_data(fd,buffer[i])!=0)                                     //      On extrait le contenu du payload du pkt
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
          size_used--;
          if(send_ack(sfd,sseqnum+1)==-1)                                       //      On renvoie le ack correspondant
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
          sseqnum++;                                                            //      On incremente sseqnum
          if(sseqnum>255)
          sseqnum=0;
        }
      }
    }
                                                                                //  Plus de ack a envoyer
    pkt_t* pkt=pkt_new();
    int err = receive_pkt(sfd,pkt); //TODO : check crc ou tr
    fflush(stdout);
    if(err==2)                                                                  //  Si le paquet recu est tronque
    {
      uint8_t pkt_seqnum=pkt_get_seqnum(pkt);
      if(seqnum_in_window(sseqnum,WINDOW_LENGTH,pkt_seqnum))                    //    Si il est dans la fenetre attendue
      {
        if(send_nack(sfd,pkt_get_seqnum(pkt))==-1)                              //      On envoie un nack
        {
          fprintf(stderr,"Error : sending nack\n");
          pkt_del(pkt);
        }
        pkt_del(pkt);
      }
    }
    else if(err==1)                                                             //  Si le paquet contient le EOF
    {
      if(send_ack(sfd,pkt_get_seqnum(pkt)+1)<0)                                 //    On envoie le dernier ack
      {
        fprintf(stderr,"Error sending final ACK\n");
        pkt_del(pkt);
        free(buffer);
        buffer = NULL;
        return -1;
      }
      pkt_del(pkt);
      free(buffer);
      return 0;                                                                 //    On termine l'execution
    }
    else if(err==0)                                                             //  Si le paquet n'est pas modifie
    {
      uint8_t pkt_seqnum=pkt_get_seqnum(pkt);
      if(seqnum_in_window(sseqnum,WINDOW_LENGTH,pkt_seqnum))                    //    Si il est dans la fenetre attendue
      {
        if(sseqnum==pkt_seqnum)                                                 //      Si c'est le premier paquet attendu
        {
          int i;
          for(i=0;i<WINDOW_LENGTH;i++)                                          //        On verifie s'il n'est pas deja dans le buffer
          {
            if(buffer[i]!=NULL)
            {
              if(pkt_get_seqnum(buffer[i])==sseqnum)                            //          S'il l'est
              {
                pkt_del(buffer[i]);                                             //            On le supprime
                size_used--;
                i=WINDOW_LENGTH;
              }
            }
          }
          if(send_ack(sfd,sseqnum+1)!=0)                                        //        On envoie un ack
          {
            fprintf(stderr,"Error : sending ack\n");
          }
          first_received=1;
          sseqnum++;
          if(sseqnum>255)
          sseqnum=0;
          if(process_data(fd,pkt)!=0)                                           //        On extrait le payload du pkt vers la sortie
          {
            fprintf(stderr,"Error : processing data\n");

            return -1;
          }
        }
        else                                                                    //      Si pas le premier paquet attendu
        {
          int boolean=0;
          int i;
          for(i=0;i<WINDOW_LENGTH && !boolean;i++)                              //        On regarde s'il est deja dans le buffer de reception
          {
            if(buffer[i]!=NULL)
            {
              if(pkt_get_seqnum(buffer[i])==pkt_seqnum)
              {
                boolean=1;
              }
            }
          }
          if(boolean)                                                           //        Si deja dans le buffer
          {
            pkt_del(pkt);
            if(first_received)                                                  //          Si pas le premier pkt
            {
              if(send_ack(sfd,sseqnum)!=0)                                      //            On renvoie juste un ack
              {
                fprintf(stderr,"Error : sending ack\n");
              }
            }
          }
          else                                                                  //        Si pas encore dans le buffer
          {
            int added=0;
            int i;
            for(i=0;i<WINDOW_LENGTH;i++)                                        //          On cherche une place vide
            {
              if(buffer[i]==NULL)                                               //            Si on a trouve une place vide
              {
                buffer[i]=pkt;                                                  //              On l'y ajoute
                i=WINDOW_LENGTH;
                added=1;
                size_used++;
              }
            }
            if(!added)                                                          //          Si pas ete ajoute
            {
              pkt_del(pkt);
              fprintf(stderr,"Error : no space on buffer - pkt discarded\n");   //            On supprime le pkt
            }
            if(first_received)
            {
              if(send_ack(sfd,sseqnum)!=0)
              {
                fprintf(stderr,"Error : sending ack\n");
              }
            }
          }
        }
      }
      /*else                                                                      //    Si pas dans la fenetre
      {
        if(first_received)
        {
          if(send_ack(sfd,sseqnum)!=0)
          {
            fprintf(stderr,"Error : sending ack\n");
          }
        }
        pkt_del(pkt);
      }*/
    }
  }                                                                             //  Fin de la boucle
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
  const char* err= real_address(first_address, &addr);                          //Stockage de l'addresse
  if(err!=NULL)
  {
    fprintf(stderr, "Wrong hostname %s: %s\n", first_address, err);
    return EXIT_FAILURE;
  }
  int sfd = create_socket(&addr, port, NULL, -1);                               //Creation d'un socket
  if (sfd < 0) {
    fprintf(stderr, "Failed to create the socket!\n");
    return EXIT_FAILURE;
  }
  if (wait_for_client(sfd) < 0)                                                 //Connexion avec le sender
  {
    fprintf(stderr,"Could not connect the socket after the first message.\n");
    close(sfd);
    return EXIT_FAILURE;
  }
  if(receive_data(sfd, filename, optionf) < 0)                                  //Lance la reception de donnees
  {
    fprintf(stderr, "Reception error\n");
    close(sfd);
    return EXIT_FAILURE;
  }
  printf("=== Data successfully received ===\n");
  printf("Number of NACK : %d\n",countNack);
  printf("Number of ACK : %d\n",countAck);
  close(sfd);
  return EXIT_SUCCESS;                                                          //Fin du programme de reception
}
