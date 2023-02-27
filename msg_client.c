
#include <arpa/inet.h>

typedef struct msg_client {
    int codereq;
    int id;
    int numfil;
    int nb;
    int datalen;
    char* data; //peut être pseudo si inscirption si codereq = 0
    int is_inscript; 
} msg_client;

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
    char msg[1024][16];
    msg = malloc(sizeof(char *) * 1024);

    //ENTETE
    //CODEREQ | ID
    int entete = htons(struc.codereq) << 11 | htons(struc.id) ;
    msg[0] = malloc(2);
    memcpy(msg[0], &entete, sizeof(entete) ); 

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
            char octet1 = malloc(9 * sizeof(char)); char octet2 = malloc(9 * sizeof(char));
            itoa(car1, octet1, 2); itoa(car2, octet2, 2);
            char* couple = strcat(octet1, octet2);
            msg[(i/2)+1] = malloc(strlen(couple));
            memcpy(msg[(i/2)+1], couple, strlen(couple) );
            free(octet1); free(octet2);
        }
        return msg;
    }

    //NUMFIL
    int numfil = htons(struc.numfil);
    msg[1] = malloc(2);
    memcpy(msg[1], &numfil, sizeof(numfil));

    //NB
    int nb = htons(struc.nb);
    msg[2] = malloc(2);
    memcpy(msg[2], &nb, sizeof(nb));

    //DATALEN | DATA[0]
    unsigned short datalen = (unsigned short)(struc.datalen);
    msg[3] = malloc(2);
    memcpy(msg[3], &datalen, sizeof(datalen));
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
            char octet1 = malloc(9 * sizeof(char)); char octet2 = malloc(9 * sizeof(char));
            itoa(car1, octet1, 2); itoa(car2, octet2, 2);
            char* couple = strcat(octet1, octet2);
            msg[(i/2)+1] = malloc(strlen(couple));
            memcpy(msg[(i/2)+4], couple, strlen(couple) );
            free(octet1); free(octet2);
    }
    return msg;
}