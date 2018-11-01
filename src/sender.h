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
#include <math.h>
#include <sys/time.h>
#include "window.c"

/*
 * Ouvre le fichier d'entree (peut etre stdin) et le copie sur le
 * payload de pkt et les envoie via le sfd passe en argument.
 * Cree une window stockant les pkt envoye et dont on a pas recu d'accuse
 * de reception et renvoie un pkt apres un temps RTT de 4 secondes
 * si l'accuse de reception n'est pas recu. Envoie egalement un pkt_t
 * de EOF et se deconnecte apres 10 secondes si l'ack de deconnexion
 * n'est pas recu.
 *
 * @sfd: Le Socket File Director via lequel envoyer/recevoir des pkt
 * @filename: Le path name du fichier d'entree
 * @optionf: booleen disant si l'option f est presente et
 *           si la sortie se fait sur un fichier et non sur stdin
 * @return: -1 en cas d'erreur
 *          0 si toutes les donnees ont bien ete traitees
 */
int send_data(int sfd, char* filename, int optionf);
