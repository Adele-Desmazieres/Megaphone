#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <arpa/inet.h>

#include "msg_serveur.h"

//Construit une structure message serveur allouée sur le tas
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

//Transforme un struct msg_client en un buffer prêt pour l'envoi réseau
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

//Transforme un buffer réseau reçu en structure msg_serveur
msg_serveur tcp_to_msgserveur(uint16_t * msg) {

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

    memcpy(ret + 1, struc.origine, 10);

    memcpy(ret + 6, struc.pseudo, 10);

    //DATA
    ret[11] = (u_int16_t)( ((int)struc.data[0] << 8) + struc.datalen );

    memcpy(ret + 12, struc.data+1, struc.datalen - 1);

    return ret;
}

//Renvoie une structure msg_billet_envoi en lisant directement depuis le sockfd spécifié
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

    origine[10] = '\0';

    ret->origine = origine;

    //PSEUDO
    char * pseudo = malloc(11);
    memset(pseudo, 0, 11);

    //On recoit les 10 prochains octets pour pseudo
    recu = recv(sockfd, pseudo, 10, 0);

    if(recu <= 0) {
        return NULL;
    }
    pseudo[10] = '\0';

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

    recu = 0;
    while(recu < datalen){

        int recuTmp;
        recuTmp = recv(sockfd, buf + recu, datalen, 0);

        if(recuTmp <= 0) {
            return NULL;
        }

        recu += recuTmp;
    }

    buf[datalen] = '\0';

    ret->data = buf;

    return ret;
}
