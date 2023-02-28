#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <arpa/inet.h>

#include "msg_client.h"

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

//Transforme un struct msg_client en un "message" pour TCP
char** msg_client_to_send(msg_client struc){

    //Taille arbitraire pour le moment?
    char ** msg = malloc(sizeof(char *) * 1024);

    //ENTETE
    //CODEREQ | ID
    int entete = htons(struc.codereq) << 11 | htons(struc.id) ;
    memcpy(&msg[0], &entete, sizeof(entete) ); 

    //En cas d'inscription
    if(struc.is_inscript){
        //PSEUDO
        for (int i = 0; i < 10; i += 2) {
            char car1 = '#';
            char car2 = '#';
            if( i < strlen(struc.data) ){
                car1 = struc.data[i];
            }
            if ( (i+1) < strlen(struc.data)){
                car2 = struc.data[i+1];
            }
            char * octet1 = malloc(9 * sizeof(char)); char * octet2 = malloc(9 * sizeof(char));
            itoa(car1, octet1, 2); itoa(car2, octet2, 2);
            char* couple = strcat(octet1, octet2);
            memcpy(&msg[(i/2)+1], couple, strlen(couple) );
            free(octet1); free(octet2);
        }
        return msg;
    }

    //NUMFIL
    int numfil = htons(struc.numfil);
    memcpy(&msg[1], &numfil, sizeof(numfil));

    //NB
    int nb = htons(struc.nb);
    memcpy(&msg[2], &nb, sizeof(nb));

    //DATALEN | DATA[0]
    unsigned short datalen = (unsigned short)(struc.datalen);
    memcpy(&msg[3], &datalen, sizeof(datalen));
    char* frst_data_byte = malloc(sizeof(char) * 9);
    itoa(struc.data[0], frst_data_byte, 2);
    memcpy(msg[3]+8, frst_data_byte, 8);
    free(frst_data_byte);
    

    //DATA restant
    for (int i = 2; i < struc.datalen; i += 2) {
            char car1 = '0';
            char car2 = '0';
            if( i < struc.datalen ){
                car1 = struc.data[i];
            }
            if ( (i+1) < struc.datalen){
                car2 = struc.data[i+1];
            }
            char * octet1 = malloc(9 * sizeof(char)); char * octet2 = malloc(9 * sizeof(char));
            itoa(car1, octet1, 2); itoa(car2, octet2, 2);
            char* couple = strcat(octet1, octet2);
            memcpy(&msg[(i/2)+4], couple, strlen(couple) );
            free(octet1); free(octet2);
    }
    return msg; 
}

msg_client * tcp_to_msgclient(char ** msg) {
    //ENTETE
    //CODEREQ | ID
    char * entete_string = msg[0];

    char * codereq_string = malloc(5);
    if (codereq_string == NULL) { perror("Erreur malloc"); return NULL; }
    strncpy(codereq_string, entete_string, 5);

    int codereq = bit_to_int(codereq_string);

    printf("Codereq : %d\n", codereq);

    char * id_string = malloc(11);
    if (id_string == NULL) { perror("Erreur malloc"); return NULL; }
    strncpy(id_string, entete_string + 5, 11);

    int id = bit_to_int(id_string);

    printf("Id : %d\n", id);

    return NULL;
}

int bit_to_int(char * bits) {
    int i = strlen(bits);
    

    return 0;
}