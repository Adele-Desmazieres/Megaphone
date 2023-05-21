#ifndef MSG_MULTICAST
#define MSG_MULTICAST

#define SIZE_MSG_ABO 22
#define SIZE_MSG_NOTIF 34

#define PORT_MULTICAST 8686

#define FIRST_MULTICAST_IP "ff12:0000:0000:0000:0000:0000:0000:0000"

#include <arpa/inet.h>

//Structure représentant le message que le client reçoit pour s'abonner à un fil
typedef struct msg_demande_abo{

    int codereq, id, numfil, nb;
    char * ip;
    
} msg_demande_abo;

//Transforme un msg_demande_abo en buffer prêt pour l'envoi réseau
u_int16_t * msg_abo_to_tcp(msg_demande_abo msg);

//Renvoie un msg_demande_abo alloué sur le tas, en lisant directement depuis le sockfd spécifié
msg_demande_abo * tcp_to_msg_abo(int sockfd);

//Structure représentant un message de notification reçu en multicast par le client
typedef struct msg_notif{
    int codereq, id, numfil;
    char * pseudo;
    char * data;
} msg_notif;

//Transforme un msg_notif en buffer prêt pour l'envoi réseau
u_int16_t * msg_notif_to_udp(msg_notif msg);

//Transforme un buffer reçu en msg_notif alloué sur le tas
msg_notif * udp_to_msg_notif(u_int16_t * oct);


//Renvoie une chaîne de caractère allouée sur le tas correspondant à l'adresse "ip" (IPv6) incrémentée de 1
char * incr_ip(char * ip);

#endif