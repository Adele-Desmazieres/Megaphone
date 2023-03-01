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

msg_client * msg_client_constr(int codereq, int id, int numfil, int nb, int datalen, char * data, int is_inscript);
uint16_t * msg_client_to_send(msg_client struc);
msg_client * tcp_to_msgclient(uint16_t * msg);

#endif