#ifndef MSG_SERVEUR
#define MSG_SERVEUR

#define SIZE_MSG_SERVEUR 6

#include <stdint.h>

//Structure représentant la première réponse du serveur
typedef struct msg_serveur {
    int codereq, id, numfil, nb;
} msg_serveur;

//Constructeur pour la structure msg_serveur, renvoie une structure allouée sur le tas
msg_serveur * msg_serveur_constr(int codereq, int id, int numfil, int nb);

//Renvoie un buffer prêt pour l'envoi réseau, depuis le msg_serveur spécifié
uint16_t * msg_serveur_to_send(msg_serveur struc);

//Transforme un buffer reçu en msg_serveur
msg_serveur tcp_to_msgserveur(uint16_t * msg);


//Structure représentant un billet envoyé pour la commande get n billets
typedef struct msg_billet_envoi {
    int numfil; 
    int datalen;
    char * origine;
    char * pseudo;
    char * data;
} msg_billet_envoi;

//Transforme un msg_billet_envoi en buffer prêt pour l'envoi réseau
uint16_t * msg_billet_to_send(msg_billet_envoi struc);

//Renvoie un msg_billet_envoi alloué sur le tas, en lisant directement depuis le sockfd spécifié
msg_billet_envoi * tcp_to_msgbillet(int sockfd);

#endif