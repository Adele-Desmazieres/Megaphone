
#include "msg_multicast.h"
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bdd_serveur.h"

u_int16_t * msg_abo_to_tcp(msg_demande_abo msg){

    u_int16_t * ret = malloc(SIZE_MSG_ABO);
    if(ret == NULL) perror(" Erreur malloc @ msg_abo_to_tcp\n");

    //ENTETE
    //CODEREQ | ID
    ret[0] = htons((msg.id << 5) + msg.codereq);

    ret[1] = htons(msg.numfil);

    ret[2] = htons(msg.nb);

    struct in6_addr adr = {0};
    
    memset(&adr,0, sizeof(adr));
    inet_pton(AF_INET6, msg.ip, &adr);


    //Potentiellement faux...
    for(int i = 0; i < 16; i+=2){
        memcpy((ret + 3 + i), adr.s6_addr+(15 - i), 2);
    }

    return ret;

}

msg_demande_abo * tcp_to_msg_abo(int sockfd){

    msg_demande_abo * ret = malloc(sizeof(msg_demande_abo));
    if(ret == NULL) perror("Erreur malloc @ tcp_to_msg_abo \n");

    u_int16_t oct[SIZE_MSG_ABO/2];
    memset(oct, 0, SIZE_MSG_ABO);

    if (recv(sockfd, oct, SIZE_MSG_ABO, 0) < 0){
        perror("recv @ tcp_to_msg_abo \n");
    }

    oct[0] = ntohs(oct[0]);
    ret->id = (oct[0] >> 5);
    ret->codereq = oct[0] - ((ret->id) << 5);

    ret->numfil = ntohs(oct[1]);
    ret->nb = ntohs(oct[2]);

    //C'est une dinguerie mais askip c'est ça
    uint8_t ip_bin[16] = {0};

    for(int i = 0; i < 8; i++){
        //A remettre dans le bon ordre
        memcpy(ip_bin+i*2, (oct + 3 + (8 - i)), 2);
    }

    struct in6_addr adr;
    memcpy(adr.s6_addr, ip_bin, 16);

    ret->ip = malloc(sizeof(char) * 40); //???

    inet_ntop(AF_INET6, &adr, ret->ip, 40);

    return ret;
}

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

msg_notif * udp_to_msg_notif(u_int16_t * oct){

    msg_notif * ret = (malloc (sizeof(msg_notif)));
    if(ret == NULL) perror("Erreur malloc @ tcp_to_msg_notif \n");



    u_int16_t trad = ntohs(oct[0]);
    ret->id = (trad >> 5);
    ret->codereq = trad - ((ret->id) << 5);
    ret->numfil = ntohs(oct[1]);

    ret->pseudo = malloc(10);
    memcpy(ret->pseudo, oct+2, 10);
    printf("TEST\n");

    ret->data = malloc(20);
    memcpy(ret->data, oct+7, 20);

    return ret;


}

void free_msg_notif(msg_notif * msg){
    free(msg->pseudo);
    free(msg->data);
    free(msg);
}

/*
int main(void){
    msg_notif test = {.codereq = 1, .id = 0, .numfil = 0, .pseudo = "LUZog", .data = "salu"};
    msg_notif * test2 = udp_to_msg_notif(msg_notif_to_udp(test));
    printf("CODEREQ == 1 ? %d \n", test2->codereq ); 
    printf("PSEUDO == LUZog ? %s \n", test2->pseudo ); 
    printf("DATA == salu %s \n", test2->data);


    free_msg_notif(test2);
}
*/
