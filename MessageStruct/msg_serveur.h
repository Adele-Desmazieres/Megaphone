#ifndef MSG_SERVEUR
#define MSG_SERVEUR

#define SIZE_MSG_SERVEUR 6

#include <stdint.h>

typedef struct msg_serveur {
    int codereq, id, numfil, nb;
} msg_serveur;

typedef struct msg_billet_envoi {
    int numfil; 
    int datalen;
    char * origine;
    char * pseudo;
    char * data;
} msg_billet_envoi;

msg_serveur * msg_serveur_constr(int codereq, int id, int numfil, int nb);
uint16_t * msg_serveur_to_send(msg_serveur struc);
msg_serveur tcp_to_msgserveur(uint16_t * msg);

uint16_t * msg_billet_to_send(msg_billet_envoi struc);
msg_billet_envoi * tcp_to_msgbillet(int sockfd);

#endif