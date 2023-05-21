
#include "msg_multicast.h"
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "../ClientServeur/bdd_serveur.h"

//Transforme un msg_demande_abo en buffer prêt pour l'envoi réseau
u_int16_t * msg_abo_to_tcp(msg_demande_abo msg){

    u_int16_t * ret = malloc(SIZE_MSG_ABO);
    if(ret == NULL) perror(" Erreur malloc @ msg_abo_to_tcp\n");

    //CODEREQ | ID
    ret[0] = htons((msg.id << 5) + msg.codereq);

    ret[1] = htons(msg.numfil);

    ret[2] = htons(msg.nb);

    struct in6_addr adr = {0};
    
    memset(&adr,0, sizeof(adr));
    inet_pton(AF_INET6, msg.ip, &adr);

    memcpy(ret + 3, &adr, 16);

    return ret;

}

//Renvoie un msg_demande_abo alloué sur le tas, en lisant directement depuis le sockfd spécifié
msg_demande_abo * tcp_to_msg_abo(int sockfd){

    msg_demande_abo * ret = malloc(sizeof(msg_demande_abo));
    if(ret == NULL) perror("Erreur malloc @ tcp_to_msg_abo \n");

    u_int16_t oct1[1] = {0};
    if (recv(sockfd, oct1, 2, 0) < 0){
        perror("recv @ tcp_to_msg_abo \n");
    }

    oct1[0] = ntohs(oct1[0]);
    ret->id = (oct1[0] >> 5);
    ret->codereq = oct1[0] - ((ret->id) << 5);

    if (ret->codereq == 31) return ret;

    u_int16_t oct[(SIZE_MSG_ABO/2) - 1];
    memset(oct, 0, SIZE_MSG_ABO - 2);

    if (recv(sockfd, oct, SIZE_MSG_ABO - 2, 0) < 0){
        perror("recv @ tcp_to_msg_abo \n");
    }

    ret->numfil = ntohs(oct[0]);
    ret->nb = ntohs(oct[1]);

    struct in6_addr adr;
    memcpy(&adr, oct+2, 16);  

    ret->ip = malloc(sizeof(char) * 40);

    inet_ntop(AF_INET6, &adr, ret->ip, 40);

    return ret;
}

//Transforme un msg_notif en buffer prêt pour l'envoi réseau
u_int16_t * msg_notif_to_udp(msg_notif msg){

    u_int16_t * ret = malloc(SIZE_MSG_NOTIF);
    if(ret == NULL) perror(" malloc @ msg_notif_to_tcp \n");

    ret[0] = htons((msg.id << 5) + msg.codereq);

    ret[1] = htons(msg.numfil);

    ret += 2;

    memcpy(ret, msg.pseudo, 10);

    char data[20];
    memset(data, '\0', 20);

    memcpy(data, msg.data, 20);

    ret += 5;

    memcpy(ret, data, 20);

    ret -= 7;

    return ret;

}

//Transforme un buffer reçu en msg_notif alloué sur le tas
msg_notif * udp_to_msg_notif(u_int16_t * oct){

    msg_notif * ret = (malloc (sizeof(msg_notif)));
    if(ret == NULL) perror("Erreur malloc @ tcp_to_msg_notif \n");

    u_int16_t trad = ntohs(oct[0]);
    ret->id = (trad >> 5);
    ret->codereq = trad - ((ret->id) << 5);
    ret->numfil = ntohs(oct[1]);

    ret->pseudo = malloc(10);
    memcpy(ret->pseudo, oct+2, 10);

    ret->data = malloc(20);
    memcpy(ret->data, oct+7, 20);

    return ret;

}

//Libère un msg_notif alloué sur le tas
void free_msg_notif(msg_notif * msg){
    free(msg->pseudo);
    free(msg->data);
    free(msg);
}

//Incrémente le caractère src (correspondant à un hexadécimal)
char incr_hexchar(char src){
    if (isdigit(src)){
        if(src - '0' == 9) return 'a';
        return ((src - '0') + 1) + '0';
    }
    switch(src){
        case 'a' : return 'b';
        case 'b' : return 'c';
        case 'c' : return 'd';
        case 'd' : return 'e';
        case 'e' : return 'f';
        case 'f' : return '0';
        default :
            printf("Erreur de char pour IPv6 \n");
            return 0;
    }
}

//Renvoie une chaîne de caractère allouée sur le tas correspondant à l'adresse "ip" (IPv6) incrémentée de 1
char * incr_ip(char * ip){

    char * ret = malloc(40);
    memcpy(ret, ip, 40);

    int char_pos = strlen(ip) - 1;
    while(ip[char_pos] == ret[char_pos]){

        if(char_pos <= 4) goto error;

        switch(ret[char_pos]){
            case ':' : char_pos--; break;
            case 'f' : ret[char_pos--] = '0'; break;
            default :
                if (!isalnum(ret[char_pos])) goto error;
                ret[char_pos] = incr_hexchar(ret[char_pos]);
        }
    }
    return ret;
    error :
        free(ret); return NULL;

}

