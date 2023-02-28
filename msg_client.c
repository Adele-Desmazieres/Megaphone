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

    printf("DEBUT\n");

    //Taille arbitraire pour le moment?
    char ** msg = malloc(sizeof(char *) * 1024);

    //ENTETE
    //CODEREQ | ID
    int entete = htons(struc.codereq) << 11 | htons(struc.id) ;
    memcpy(&msg[0], &entete, sizeof(entete) ); 

    printf("Probleme entete\n");

    //En cas d'inscription
    if(!struc.is_inscript){
        //PSEUDO
        printf("INSCRIPTION\n");
        for (int i = 0; i < 10; i += 2) {
            char car1 = '#';
            char car2 = '#';

            if( i < strlen(struc.data) ){
                car1 = struc.data[i];
            }
            if ( (i+1) < strlen(struc.data)){
                car2 = struc.data[i+1];
            }

            char * octet1[9]; char * octet2[9];
            if (octet1 == NULL || octet2 == NULL) { perror("malloc"); return NULL; }

            int_to_bit_string(car1, octet1);
            int_to_bit_string(car2, octet2);
            //itoa(car1, octet1, 2); itoa(car2, octet2, 2); 

            char* couple = strcat(octet1, octet2);
            memcpy(&msg[(i/2)+1], couple, strlen(couple) );
        }

        return msg;
    }

    printf("Numfil\n");

    //NUMFIL
    int numfil = htons(struc.numfil);
    memcpy(&msg[1], &numfil, sizeof(numfil));

    printf("Nb\n");

    //NB
    int nb = htons(struc.nb);
    memcpy(&msg[2], &nb, sizeof(nb));

    printf("Data\n");

    //DATALEN | DATA[0]
    unsigned short datalen = (unsigned short)(struc.datalen);
    memcpy(&msg[3], &datalen, sizeof(datalen));
    char* first_data_byte = malloc(sizeof(char) * 9);

    int_to_bit_string(struc.data[0], first_data_byte);
    //itoa(struc.data[0], first_data_byte, 2);

    memcpy(msg[3]+8, first_data_byte, 8);
    free(first_data_byte);

    printf("Data restant\n");
    
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
            printf("Car1 : %c, Car2 : %c", car1, car2);
            int_to_bit_string(car1, octet1);
            int_to_bit_string(car2, octet2);
            //itoa(car1, octet1, 2); itoa(car2, octet2, 2); 

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

void int_to_bit_string(int n, char * bit_string) {
    printf("Ascii : %c, Nb : %d\n", n + '0', n);

    //On met à 0 toutes les cases de notre bit_string par défaut.
    memcpy(bit_string, "0", strlen(bit_string));
    bit_string[strlen(bit_string) - 1] = '\0';
    int index = strlen(bit_string) - 2;

    //On va jusqu'à la fin du nombre pour rajouter bit par bit les puissances de 2.
    while(n > 0) {
        if (index % 2 == 0) {
            bit_string[index] = '0';
        }
        else {
            bit_string[index] = '1';
        }
        index -= 1;
        n /= 2;
    }
}

int bit_to_int(char * bits) {
    int i = strlen(bits);
    
    return 0;
}