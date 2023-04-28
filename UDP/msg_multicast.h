#ifndef MSG_MULTICAST
#define MSG_MULTICAST

#define SIZE_MSG_ABO 22
#define SIZE_MSG_NOTIF 34

#define MULTICAST_INTERFACE "wlp3s0"
#define PORT_MULTICAST 8686

#define FIRST_MULTICAST_IP "ff12:0000:0000:0000:0000:0000:0000:0000"

#include <arpa/inet.h>

typedef struct msg_demande_abo{

    int codereq, id, numfil, nb;
    char * ip;
    

} msg_demande_abo;

u_int16_t * msg_abo_to_tcp(msg_demande_abo msg);
msg_demande_abo * tcp_to_msg_abo(int sockfd);

typedef struct msg_notif{
    int codereq, id, numfil;
    char * pseudo;
    char * data;
} msg_notif;

u_int16_t * msg_notif_to_udp(msg_notif msg);
msg_notif * udp_to_msg_notif(u_int16_t * oct);

char * incr_ip(char * ip);



#endif