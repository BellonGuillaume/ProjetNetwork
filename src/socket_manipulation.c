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
