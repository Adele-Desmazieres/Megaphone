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

            ret[(i/2) + 1] = htons(((int)car2  << 8) + (int)car1);
    }

    //PSEUDO
    for (int i = 0; i < 10; i += 2) {
            //En cas de dépassement, on remplit de #
            char car1 = ( i < strlen(struc.pseudo)) ? struc.pseudo[i] : '#';
            char car2 = ( i+1 < strlen(struc.pseudo)) ? struc.pseudo[i+1] : '#';

            ret[(i/2) + 6] = htons(((int)car2  << 8) + (int)car1);
    }

    //DATA
    ret[11] = htons( ((int)struc.data[0] << 8) + struc.datalen );

    //DATA restant
    int data_pointer = 1;
    for (int i = 12; data_pointer < strlen(struc.data); data_pointer+= 2, i++) {
        //En cas de dépassement on remplit par des #, comme pour le pseudo
            char car1 = ( data_pointer < strlen(struc.data)) ? struc.data[data_pointer] : '#';
            char car2 = ( data_pointer+1 < strlen(struc.data)) ? struc.data[data_pointer+1] : '#';

            ret[i] = htons(((int)car2 << 8) + (int)car1);
    }

    return ret;
}

msg_billet_envoi * tcp_to_msgbillet(uint16_t * msg){

    msg_billet_envoi * ret = malloc(sizeof(msg_billet_envoi));

    //NUMFIL
    ret->numfil = ntohs(msg[0]);

    //ORIGINE
    char * origine = malloc(11 * sizeof(char));
        int origine_pointer = 0;
        for(int i = 1; origine_pointer < 11; origine_pointer += 2, i++){
            uint16_t cars = ntohs(msg[i]);
            char car1 = (char)(cars >> 8);
            char car2 = (char)(cars - (car1 << 8));

            origine[origine_pointer] = car2;
            origine[origine_pointer+1] = car1;
        }
        origine[10] = '\0';

        //On élimine les #
        char * realorigine = get_real_name(origine);
        free(origine);

    ret->origine = realorigine;

    //PSEUDO
    char * pseudo = malloc(11 * sizeof(char));
        int pseudo_pointer = 0;
        for(int i = 6; pseudo_pointer < 11; pseudo_pointer += 2, i++){
            uint16_t cars = ntohs(msg[i]);
            char car1 = (char)(cars >> 8);
            char car2 = (char)(cars - (car1 << 8));

            pseudo[pseudo_pointer] = car2;
            pseudo[pseudo_pointer+1] = car1;
        }
        pseudo[10] = '\0';

    //On élimine les #
        char * realpseudo = get_real_name(origine);
        free(origine);
    
    ret->pseudo = realpseudo;

    //DATALEN ET DATA
    uint16_t datalenData = ntohs(msg[11]);
    char car1 = (char)(datalenData >> 8);
    int datalen = (datalenData - ((int)car1 << 8));

    //Si datalen = 0, erreur...
    if(datalen <= 0) { perror("Null data, exiting\n"); exit(1); }

    //On alloue une chaine de taille datalen+1
    char * finalData = malloc((datalen+1) * sizeof(char) );
    //On ajoute le premier caractère
    finalData[0] = car1;
    //printf("%c \n", finalData[0]);
    int data_pointer = 1;

        for(int i = 12; data_pointer < datalen ; data_pointer += 2, i++){

            uint16_t cars = ntohs(msg[i]);
            
            //On prends les deux caractères représentés par l'octet...
            uint16_t car1_int = (cars >> 8); 
            char car1 = (char)car1_int;
            char car2 = (data_pointer+1 < datalen) ? (char)(cars - (car1_int << 8)) : '\0';

            //printf("%c %c\n", car1, car2);

            //... or si i ou i+1 dépasse datalen, c'est qu'on a fini
            finalData[data_pointer] = car2;
            if(car2 == '\0') break;
            finalData[data_pointer+1] = car1;
            if(car1 == '\0') break;

        }
    
    ret->datalen = datalen;

    //A FREE
    ret->data = finalData;

    return ret;
}
