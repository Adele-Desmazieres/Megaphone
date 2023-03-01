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
char ** msg_client_to_send(msg_client struc);
msg_client * tcp_to_msgclient(char ** msg);
void int_to_bit_string(int n, char * bit_string, size_t length);
int charbit_to_int(char * bits, size_t length);

#endif