#ifndef MSG_CLIENT
#define MSG_CLIENT

typedef struct msg_client {
    int codereq;
    int id;
    int numfil;
    int nb;
    int datalen;
    char * data; //peut être pseudo si inscirption si codereq = 0
    int is_inscript; 
} msg_client;

//Constructeur pour la structure msg_client, renvoie un pointeur
msg_client * msg_client_constr(int codereq, int id, int numfil, int nb, int datalen, char * data, int is_inscript);

//Transforme une structure msg_client en buffer prêt pour l'envoi réseau
u_int16_t * msg_client_to_send(msg_client struc);

//Renvoie une structure msg_client allouée, en lisant directement depuis le sockfd spécifié
msg_client * tcp_to_msgclient(int sockfd);

#endif