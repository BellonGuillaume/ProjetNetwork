#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <ctype.h>
#include "socket_manipulation.c"
#include "commonlib.c"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * Cree un pkt de type ack, l'initialise et l'envoie sur
 * un socket via le sfd donne.
 *
 * @sfd: Le Socket File Director sur lequel envoyer l'ack
 * @seqnum: Le nouveau numero de sequence que le receiver s'attend a recevoir
 * @return: -1 en cas d'erreur
 *          0 si tout s'est bien passe
 */
int send_ack(int sfd, int seqnum);

/*
 * Cree un pkt de type nack, l'initialise et l'envoie sur
 * un socket via le sfd donne.
 *
 * @sfd: Le Socket File Director sur lequel envoyer l'ack
 * @seqnum: Le nouveau numero de sequence que le receiver s'attend a recevoir
 * @return: -1 en cas d'erreur
 *          0 si tout s'est bien passe
 */
int send_nack(int sfd, int seqnum);

/*
 * Copie le payload du pkt donne en argument et l'ecrit sur la sortie demande.
 *
 * @fd: Le File Director de sortie
 * @pkt: Le paquet duquel prendre le payload
 * @return: 0 si tout s'est bien passe
 */
int process_data(int fd, pkt_t* pkt);

/*
 * Recoit les pkt d'un sender et les traite pour extraire
 * leur payload sur la sortie demande. Renvoie aussi des ack
 * et des nack au sender en fonction du pkt recu. Code implemente
 * en selective repeat.
 *
 * @sfd: Le Socket File Director via lequel recevoir/envoyer des pkt
 * @filename: Le path name du fichier de sortie
 * @optionf: booleen disant si l'option f est presente et
 *           si la sortie se fait sur un fichier et non sur stdin
 * @return: -1 en cas d'erreur
 *          0 si toutes les donnees ont bien ete traitees
 */
int receive_data(int sfd, char* filename, int optionf);
