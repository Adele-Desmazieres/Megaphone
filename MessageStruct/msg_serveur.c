#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <arpa/inet.h>

#include "msg_serveur.h"

msg_serveur * msg_serveur_constr(int codereq, int id, int numfil, int nb){
    msg_serveur * ret = malloc(sizeof(msg_serveur));
    if(ret == NULL){
        perror("Erreur malloc @ msg_serveur l16");
        exit(-1);
    }

    ret->id = id;
    ret->codereq = codereq;
    ret->nb = nb;
    ret->numfil = numfil;

    return ret;
}

//Transforme un struct msg_client en un "message" pour TCP
uint16_t * msg_serveur_to_send(msg_serveur struc){

    //Taille: 6 octets si inscription sinon, dépend de la taille du texte
    uint16_t * msg = malloc(6);
    if(msg == NULL) perror("Erreur msg_serveur");

    //ENTETE
    //CODEREQ | ID
    msg[0] = htons((struc.id << 5) + struc.codereq) ;

    //NUMFIL
    msg[1] = htons(struc.numfil);

    //NB
    msg[2] = htons(struc.nb);
    
    return msg; 
}

msg_serveur tcp_to_msgserveur(uint16_t * msg) {

    //ENTETE
    //CODEREQ | ID
    uint16_t entete = ntohs(msg[0]);
    int id = (entete >> 5);
    int codereq = entete - (id << 5);

    //NUMFIL ET NB
    int numfil = ntohs(msg[1]);
    int nb = ntohs(msg[2]);

    msg_serveur retour = {codereq, id, numfil, nb};
    return retour;
}

//Transforme un message représentant un billet en message TCP
uint16_t * msg_billet_to_send(msg_billet_envoi struc){
    uint16_t * ret = malloc(24 + (strlen(struc.data ) - 1));


    //NUMFIL
    ret[0] = htons(struc.numfil);

    //ORIGINE
    for (int i = 0; i < 10; i += 2) {
            //En cas de dépassement, on remplit de #
            char car1 = ( i < strlen(struc.origine)) ? struc.origine[i] : '#';
            char car2 = ( i+1 < strlen(struc.origine)) ? struc.origine[i+1] : '#';

            ret[(i/2) + 1] = (u_int16_t)(((int)car2  << 8) + (int)car1);
    }

    //PSEUDO
    for (int i = 0; i < 10; i += 2) {
            //En cas de dépassement, on remplit de #
            char car1 = ( i < strlen(struc.pseudo)) ? struc.pseudo[i] : '#';
            char car2 = ( i+1 < strlen(struc.pseudo)) ? struc.pseudo[i+1] : '#';

            ret[(i/2) + 6] = (u_int16_t)(((int)car2  << 8) + (int)car1);
    }

    //DATA
    ret[11] = (u_int16_t)( ((int)struc.data[0] << 8) + struc.datalen );

    //DATA restant
    int data_pointer = 1;
    for (int i = 12; data_pointer < strlen(struc.data); data_pointer+= 2, i++) {
        //En cas de dépassement on remplit par des #, comme pour le pseudo
            char car1 = ( data_pointer < strlen(struc.data)) ? struc.data[data_pointer] : '#';
            char car2 = ( data_pointer+1 < strlen(struc.data)) ? struc.data[data_pointer+1] : '#';

            ret[i] = (u_int16_t)(((int)car2 << 8) + (int)car1);
    }

    return ret;
}

msg_billet_envoi * tcp_to_msgbillet(int sockfd){

    msg_billet_envoi * ret = malloc(sizeof(msg_billet_envoi));
    if(ret == NULL) return NULL;

    u_int16_t oct[1];
    memset(oct, 0, 2);

    //On recoit les 2 prochains octets pour Numfil
    int recu = recv(sockfd, oct, 2, 0);

    if(recu <= 0) {
        return NULL;
    }

    //NUMFIL
    ret->numfil = ntohs(oct[0]);

    //ORIGINE
    char * origine = malloc(11);
    memset(origine, 0, 11);

    //On recoit les 10 prochains octets pour origine
    recu = recv(sockfd, origine, 10, 0);

    if(recu <= 0) {
        return NULL;
    }

    ret->origine = origine;

    //PSEUDO
    char * pseudo = malloc(11);
    memset(pseudo, 0, 11);

    //On recoit les 10 prochains octets pour pseudo
    recu = recv(sockfd, pseudo, 10, 0);

    if(recu <= 0) {
        return NULL;
    }

    ret->pseudo = pseudo;

    //DATALEN
    
    u_int8_t dtlen[1];
    memset(dtlen, 0, 1);

    //On recoit le prochain octet pour datalen
    recu = recv(sockfd, dtlen, 1, 0);

    if(recu <= 0) {
        return NULL;
    }

    int datalen = (int)dtlen[0];
    ret->datalen = datalen;

    //DATA
    char * buf = malloc(datalen + 1);
    if (buf == NULL) return NULL;
    memset(buf, 0, datalen+1);

    //On recoit les prochains octets restants pour data
    recu = recv(sockfd, buf, datalen, 0);

    if(recu <= 0) {
        return NULL;
    }

    buf[datalen] = '\0';

    ret->data = buf;

    return ret;
}
