#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/types.h>
#include "sender.h"

struct __attribute__((__packed_)) pkt {
    uint8_t window:5;
    uint8_t tr:1;
    uint8_t type:2;
    uint8_t seqnum;
    uint16_t length;
    uint32_t timestamp;
    uint32_t crc1;
    char* payload;
    uint32_t crc2;
};

int main (int argc, char* argv[])
{
	char* send_path = NULL;
	int optionf = 0;
	bool redirection = 0;
	if (argc < 4){
		fprintf(stderr, "Pas assez d'arguments\n");
		return EXIT_FAILURE;
	}
	if (*argv[1].equals('-')){
		if (*(argv[1]+1).equals('f')){
			send_path = argv[2];
			optionf = 1;
		}
		else {
			fprintf(stderr, "Mauvaise option\n");
			return EXIT_FAILURE;
		}
	}
	else {
		if (*argv[3].equals('<')) {
			send_path = argv[4];
			redirection = 1;
		}
	}
	char* first_adresse = argv[(optionf*2)+1];
	int port = atoi(argv[(optionf*2)+2]);
	struct sockaddr_in6 addr;
	const char *err = real_address(adresse, &addr);
	if (err) {
		fprintf(stderr, "Probleme avec l'hostname %s: %s\n", adresse, err);
		return EXIT_FAILURE;
	}
	int sfd = encoder(&addr,port);
	if(sfd < 0) {
		fprintf(stderr, "Erreur dans l'encodage\n");
		return EXIT_FAILURE;
	}
	if(envoyer(sfd, send_path, optionf, redirection) < 0) {
		fprintf(stderr, "Erreur dans l'envoi\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;

}

int encoder(struct sockaddr_in6 *addr, int port){   
	int sfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    	if(sfd == -1) {
        	fprintf(stderr,"Erreur création socket\n");
        	return -1;
    	}
        
    	size_t addrlen = sizeof(struct sockaddr_in6);
	if(port > 0) {
        	addr->sin6_port=htons(port);
    	}
    
    
    	if(addr!=NULL) {
        	if(bind(sfd,(struct sockaddr*) addr, addrlen)!=0) {
            		fprintf(stderr,"error bind\n");
            	return -1;
        	}
    	}
   
    
    return sfd;
}

int envoyer(int sfd, char* send_path, int optionf, bool redirection){
}
