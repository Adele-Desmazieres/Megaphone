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

msg_serveur * tcp_to_msgserveur(uint16_t * msg) {

    //ENTETE
    //CODEREQ | ID
    uint16_t entete = ntohs(msg[0]);
    int id = (entete >> 5);
    int codereq = entete - (id << 5);

    //NUMFIL ET NB
    int numfil = ntohs(msg[1]);
    int nb = ntohs(msg[2]);

    return msg_serveur_constr(codereq, id, numfil, nb);
}
