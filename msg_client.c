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
uint16_t * msg_client_to_send(msg_client struc){

    //Taille: 6 octets si inscription sinon, dépend de la taille du texte
    uint16_t * msg = (struc.is_inscript) ? (sizeof(uint16_t) * 6) : (sizeof(uint16_t) * (4 + (strlen(struc.data) - 1) /2) );

    //ENTETE
    //CODEREQ | ID
    msg[0] = htons(struc.codereq) << 11 | htons(struc.id) ;

    //En cas d'inscription
    if(!struc.is_inscript){
        //PSEUDO
        for (int i = 0; i < 10; i += 2) {
            //En cas de dépassement, on remplit de #
            char car1 = ( i < strlen(struc.data)) ? struc.data[i] : '#';
            char car2 = ( i+1 < strlen(struc.data)) ? struc.data[i+1] : '#';

            msg[(i/2) + 1] = htons(((int)car1  << 8) + (int)car2);
        }

        return msg;
    }

    printf("Numfil\n");

    //NUMFIL
    msg[1] = htons(struc.numfil);

    printf("Nb\n");

    //NB
    msg[2] = htons(struc.nb);

    printf("Data\n");

    //DATALEN | DATA[0]
    msg[3] = htons( (struc.datalen << 8) + (int)struc.data[0] );

    printf("Data restant\n");
    
    //DATA restant
    for (int i = 2; i < struc.datalen; i += 2) {
        //En cas de dépassement on remplit par des #, comme pour le pseudo
            char car1 = ( i < strlen(struc.data)) ? struc.data[i] : '#';
            char car2 = ( i+1 < strlen(struc.data)) ? struc.data[i+1] : '#';

            msg[(i/2) + 4] = htons(((int)car1 << 8) + (int)car2);
    }
    return msg; 
}

//Elimine les # en fin de chaine, alloue la valeur de retour
char * get_real_name(const char * placeholder){
    int i = 0;
    for(; *placeholder != '\0' && *placeholder != '#'; placeholder++; i++){}
    char * ret = malloc(sizeof(char) * (i+1));
    strncat(ret, placeholder, i);

    return ret;
}

msg_client * tcp_to_msgclient(uint16_t * msg) {
    printf("Debut\n");

    //ENTETE
    //CODEREQ | ID
    uint16_t entete = ntohs(msg[0]);
    int codereq = (entete >> 11);
    int id = entete - (codereq << 11);

    if( codereq == 1 ) {
        char * pseudo = malloc(11 * sizeof(char));
        for(int i = 1; i < 6; i++){
            uint16_t cars = ntohs(msg[i]);
            char car1 = (char)(cars >> 8);
            char car2 = (char)(cars - (car1 << 8));
            pseudo[((i-1)*2)] = car1;
            pseudo[((i-1)*2)+1] = car2;
        }
        pseudo[10] = '\0';

        //On élimine les #
        char * realpseudo = get_real_name(pseudo);
        free(pseudo);

        return msg_client_constr(codereq, id, 0, 0 , 0 , realpseudo, 1);
    }

    //NUMFIL ET NB
    int numfil = ntohs(msg[1]);
    int nb = ntohs(msg[2]);

    //DATA
    uint16_t datalenData = ntohs(msg[3]);
    int datalen = (datalenData >> 8);
    char car1 = (char)(datalenData - (datalen << 8));

    //Si datalen = 0, erreur...
    if(datalen == 0) { perror("Null data, exiting\n"); exit(1); }

    //On alloue une chaine de taille datalen+1
    char * finalData = malloc((datalen+1) * sizeof(char) );
    //On ajoute le premier caractère
    finalData[0] = car1;

        for(int i = 1; i < datalen; i++){

            uint16_t cars = ntohs(msg[i/2]);
            
            //On prends les deux caractères représentés par l'octet...
            char car1 = (i < datalen) ? (char)(cars >> 8) : '\0';
            char car2 = (i+1 < datalen) ? (char)(cars - (car1 << 8)) : '\0';

            //... or si i ou i+1 dépasse datalen, c'est qu'on a fini
            finalData[i] = car1;
            if(car1 == '\0') break;
            finalData[i+1] = car2;
            if(car2 == '\0') break;
        }

    return msg_client_constr(codereq, id, numfil, nb, datalen, finalData, 0);
}
