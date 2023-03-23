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

void print_2bytes(char* bytes);
msg_client * msg_client_constr(int codereq, int id, int numfil, int nb, int datalen, char * data, int is_inscript);
u_int16_t * msg_client_to_send(msg_client struc);
char * get_real_name_client(const char * placeholder);
msg_client * tcp_to_msgclient(int sockfd);

#endif