#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <arpa/inet.h>

#include "msg_client.h"

void print_2bytes(char* bytes){
    for(int i = 0; i < 16; i++){
        printf("%c", bytes[i]);
    }
    printf("\n");
}

msg_client* msg_client_constr(int codereq, int id, int numfil, int nb, int datalen, char* data, int is_inscript){
    msg_client *ret = malloc(sizeof(msg_client));
    if(ret == NULL){
        perror("Erreur malloc @ msg_client l16");
        exit(-1);
    }

    ret->data = data;
    ret->datalen = datalen;
    ret->id = id;
    ret->codereq = codereq;
    ret->is_inscript = is_inscript;
    ret->nb = nb;
    ret->numfil = numfil;

    return ret;
}

//Lit les 2 premiers octets d'un message TCP (codereq et ID), crée une structure en conséquence
msg_client* tcp_to_msg_clientreq(int sockfd){

    uint16_t oct[1];
    memset(oct, 0, 2);

    int recu = recv(sockfd, oct, 2, 0);

    if(recu <= 0) {
        return NULL;
    }

    oct[0] = ntohs(oct[0]);
    int id = (oct[0] >> 5);
    int codereq = oct[0] - ((id) << 5);
    //printf("TEST2 %d %d\n", id, codereq);

    return msg_client_constr(codereq,id,0,0,0,NULL, (codereq == 1));

}

//Lit le pseudo depuis sockfd, stocke dans la structure msg, renvoie -1 en cas d'erreur ou 0 en succes
int lire_pseudo_depuistcp(int sockfd, msg_client * msg){

    char * pseudo = malloc(11 * sizeof(char));
    memset(pseudo, 0, 11);

    //On recoit les 10 prochains octets pour le pseudo
    int recu = recv(sockfd, pseudo, 10, 0);

    if(recu <= 0) {
        return -1;
    }

    pseudo[10] = '\0';
    //printf("pseudo1 : %s\n", pseudo);

    msg->data = pseudo;

    return 0;    
}

//Lit depuis la socket jusqu'à datalen, stocke les informations du header dans msg
int lire_header_until_datalen(int sockfd, msg_client * msg){

    u_int16_t oct[3];
    memset(oct, 0, 6);

    //On recoit les 5 prochains octets 2 pour numfil, 2 pour nb et 1 pour datalen
    int recu = recv(sockfd, oct, 5, 0);

    if(recu <= 0) {
        close(sockfd);
        return -1;
    }

    //NUMFIL ET NB
    int numfil = ntohs(oct[0]);
    int nb = ntohs(oct[1]);

    //DATA
    u_int16_t datalenData = ntohs(oct[3]);
    int car1 = (datalenData >> 8);
    int datalen = (datalenData - ((int)car1 << 8));

    msg->numfil = numfil;
    msg->nb = nb;
    msg->datalen = datalen;

    return 0;

}

//Lit DATA restant depuis une socket, à utiliser seulement si on a déjà lu tout le reste et qu'il ne reste que DATA!
int lire_data_depuistcp(int sockfd, msg_client * msg, int datalen){

    char * buf = malloc(sizeof(char) * (datalen+1) );
    memset(buf, 0, datalen+1);

    int recu = recv(sockfd, buf, datalen, 0);

    if(recu <= 0) {
        close(sockfd);
        return -1;
    }

    buf[datalen] = '\0';

    msg->data = buf;

    return 0;
    
}

//Lit directement depuis sockfd, et stocke les informations dans une structure qu'il renvoie
msg_client * tcp_to_msgclient(int sockfd) {

    msg_client * ret = tcp_to_msg_clientreq(sockfd);


    if(ret == NULL) return NULL;


    if (ret->is_inscript){

        if(lire_pseudo_depuistcp(sockfd, ret) < 0) return NULL;
        else return ret;

    }
    
    if(lire_header_until_datalen(sockfd, ret) < 0) return NULL;

    if(lire_data_depuistcp(sockfd, ret, ret->datalen) < 0) return NULL;

    return ret;
}



//Transforme un struct msg_client en un "message" pour TCP
u_int16_t * msg_client_to_send(msg_client struc){

    //Taille: 6 octets si inscription sinon, dépend de la taille du texte
    u_int16_t * msg = (struc.is_inscript) ? malloc(sizeof(u_int16_t) * 6) : malloc(sizeof(u_int16_t) * (4 + (strlen(struc.data) - 1) /2) );

    //ENTETE
    //CODEREQ | ID
    msg[0] = htons((struc.id << 5) + struc.codereq) ;

    //En cas d'inscription
    if(struc.is_inscript){
        //PSEUDO
        //printf("Taille du pseudo = %d\n", strlen(struc.data));
        for (int i = 0; i < 10; i += 2) {
            //En cas de dépassement, on remplit de #
            char car1 = ( i < strlen(struc.data)) ? struc.data[i] : '#';
            char car2 = ( i+1 < strlen(struc.data)) ? struc.data[i+1] : '#';

            //printf("%c %c \n", car1, car2);

            msg[(i/2) + 1] = (u_int16_t)(((int)car2  << 8) + (int)car1);
        }

        return msg;
    }

    //NUMFIL
    msg[1] = htons(struc.numfil);

    //NB
    msg[2] = htons(struc.nb);

    //DATALEN | DATA[0]
    msg[3] = htons( ((int)struc.data[0] << 8) + struc.datalen );
    
    //DATA restant
    int data_pointer = 1;
    for (int i = 4; data_pointer < strlen(struc.data); data_pointer+= 2, i++) {
        //En cas de dépassement on remplit par des #, comme pour le pseudo
            char car1 = ( data_pointer < strlen(struc.data)) ? struc.data[data_pointer] : '#';
            char car2 = ( data_pointer+1 < strlen(struc.data)) ? struc.data[data_pointer+1] : '#';

            //printf("%c %c \n", car1, car2);
            msg[i] = htons(((int)car2 << 8) + (int)car1);

            //printf("%d \n", msg[i]);
            
    }
    return msg; 
}

//Elimine les # en fin de chaine, alloue la valeur de retour
char * get_real_name_client(const char * placeholder){
    int i = 0;
    for(; *placeholder != '\0' && *placeholder != '#'; placeholder++, i++){}
    char * ret = malloc(sizeof(char) * (i+1));
    strncat(ret, placeholder-i, i);

    return ret;
}

