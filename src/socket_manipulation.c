#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/poll.h>
#include "packet_interface.h"

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
        fprintf(stderr,"Error creating the socket\n");
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
            fprintf(stderr,"Error binding\n");
            return -1;
        }
        //fprintf(stderr,"bind OK\n");
    }

    if(dest_addr!=NULL)
    {
        if(connect(sfd,(struct sockaddr*) dest_addr, addrlen)!=0)
        {
            fprintf(stderr,"Error connecting\n");
            return -1;
        }
        //fprintf(stderr,"connect OK\n");
    }

    return sfd;
}

//Fait avec Louis Colin
/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd){

struct sockaddr_in6 src_addr;
socklen_t addrlen=sizeof(struct sockaddr_in6);
ssize_t nread;
nread=recvfrom(sfd,NULL,0,MSG_PEEK,(struct sockaddr *) &src_addr,&addrlen);
if(nread==-1){
    //fprintf(stderr,"Error recvfrom wait_for_client\n");
    return -1;
}
if(connect(sfd,(struct sockaddr *) &src_addr,addrlen)==-1){
    //fprintf(stderr,"Error connect wait_for_client\n");
    return -1;
}
    //fprintf(stderr,"===Connected after first message - wait_for_client\n");
return 0;
}

int send_buf(int sfd, char* buf, size_t len)
{
  //printf("sendbuf\n");
  int length;
  int totalLength;
  for(totalLength=0;totalLength<len;totalLength+=length)
  {
    length=write(sfd,buf,len);
    //printf("write\n");
    if(length<0)
    {
      fprintf(stderr,"Error writing on the socket\n");
      return -1;
    }
  }
  return 0;
}

int send_pkt(int sfd, pkt_t* pkt)
{
  //printf("send pkt\n");
  char temp[1024];
  size_t length=1024;
  if(pkt_encode(pkt,temp,&length)!=PKT_OK)
  {
    fprintf(stderr,"Encoding error\n");
    return EXIT_FAILURE;
  }
  //printf("encoding ok\n");
  char buf[length];
  memcpy(buf,temp,length);
  return send_buf(sfd,buf,(size_t) length);
}

int receive_buf(int sfd, char* buf, int* len)
{
  //printf("Waiting for receive\n");
    int ret=-1;
    *len=0;
        struct pollfd fds[1];

        fds[0].fd=sfd;
        fds[0].events=POLLIN;
        //printf("about to poll\n");
        ret = poll(fds, 1, 1 );

        if (ret<0)
        {
            fprintf(stderr,"select error : ");
            fprintf(stderr,"%s\n", strerror(errno));
            return -1;
        }

        if (fds[0].revents & POLLIN)
        {
            int length=read(sfd, buf, 1024);

            if(length<0)
            {
                fprintf(stderr,"read error : ");
                fprintf(stderr,"%s\n", strerror(errno));
                return -1;
            }
            *len+=length;
            return 0;
        }
        return 3; //Rien à lire
}

/*
 * Read paquets from the socket.
 * @PRE: pkt is not malloced (created) yet.
 * @RETURN : 0 if paquet received correctily, -1 if error, 2 if packet is truncated and 1 if EOF reached.
 */
int receive_pkt(int sfd, pkt_t* pkt)
{
  int len=1024;
  char buf[1024];
  //printf("About to receive_buf\n");
  int signal=receive_buf(sfd,buf,&len);
  if(signal<0)
  {
    fprintf(stderr,"Error receiving\n");
    return -1;
  }
  if(signal==3)
  {
    return 3;
  }
  pkt_status_code err = pkt_decode(buf,len,pkt);
  if(err!=PKT_OK)
  {
    if(err==E_TR)
    {
      fprintf(stderr,"Packet truncated\n");
      return 2;
    }
    //fprintf(stderr,"Error decoding\n");
    return -1;
  }
  if((pkt_get_length(pkt)==0) && (pkt_get_type(pkt)==PTYPE_DATA))
  {
    return 1;
  }
  //printf("décodé\n");
  return 0;
}
