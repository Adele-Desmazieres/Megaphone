#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h> 

#include "liste_paquets.h"

#define SIZE_PAQ 512

paquet * paquet_constr(int codereq, int id, int numbloc, char * data, paquet * prev, paquet * next) {
    paquet * paq = malloc(sizeof(paquet));
    if (paq == NULL) return NULL;

    paq -> codereq = codereq;
    paq -> id = id;
    paq -> numbloc = numbloc;
    paq -> data = data;
    paq -> prev = NULL;
    paq -> next = NULL;
    return paq;
}

u_int16_t * paquet_to_udp(paquet paq) {
    //Taille: 6 octets si inscription sinon, dépend de la taille du texte
    u_int16_t * msg = malloc(sizeof(u_int16_t) * (4 + (SIZE_PAQ) /2) );
    if (msg == NULL) return NULL;

    //ENTETE

    //CODEREQ | ID
    msg[0] = htons((paq.id << 5) + paq.codereq);

    //NUMBLOC
    msg[1] = htons(paq.numbloc);

    //DATA
    for (int i = 2; i < SIZE_PAQ; i += 2) {
        //En cas de dépassement, on remplit de #
        char car1 = ( i < strlen(paq.data)) ? paq.data[i] : '\0';
        char car2 = ( i+1 < strlen(paq.data)) ? paq.data[i+1] : '\0';

        msg[(i/2) + 1] = (u_int16_t)(((int)car2  << 8) + (int)car1);
    }

    return msg;
}

paquet * udp_to_paquet(uint16_t * msg) {

    //ENTETE
    //CODEREQ | ID
    uint16_t entete = ntohs(msg[0]);
    int id = (entete >> 5);
    int codereq = entete - (id << 5);

    //NUMFIL ET NB
    int numbloc = ntohs(msg[1]);

    //DATA
    char * data = malloc(SIZE_PAQ);
    if (data == NULL) return NULL;
    memset(data, '\0', SIZE_PAQ);

    int r = read(0, msg+2, 512);
    //strncpy(data, msg[2], strlen(msg[2]));
    memcpy(data, msg+2, r);

    return paquet_constr(codereq, id, numbloc, data, NULL, NULL);
}

paquet * pop_paquet(liste_paquets * liste) {
    if (liste -> first == NULL) return NULL;

    paquet * paq = liste -> first;
    liste -> first = paq -> next;

    return paq;
}

/*
    Ajoute en fonction du numéro de bloc à la liste.
*/
void push_paquet(liste_paquets * liste, paquet * paq) {
    if (liste -> first == NULL) {
        liste -> first = paq;
    }
    else if (paq -> numbloc < liste -> first -> numbloc) {
        liste -> first -> prev = paq;
        paq -> next = liste -> first;

        liste -> first = paq;
    }
    else {
        paquet * courant = liste -> first;
        while (courant -> next != NULL && paq -> numbloc > courant -> numbloc) {
            courant = courant -> next;
        }
        paq -> prev = courant -> prev;
        paq -> next = courant;
        courant -> prev = paq;
    }
}

void free_paquet(paquet * paq) {
    free(paq -> data);
    free(paq);
}

void free_liste_paquets(liste_paquets * liste) {
    paquet * courant = liste -> first;
    while(courant != NULL) {
        free(courant -> data);
        paquet * tmp = courant;
        courant = tmp -> next;
        free(tmp);
    }

    free(liste);
}

int ecrire_dans_fichier_udp(char * file_name, liste_paquets * liste_paq) {
    //On ouvre le fichier et le créé si besoin.
    int fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0640);
    if (fd == -1) {
        perror("open");
        free_liste_paquets(liste_paq);
        return -1;
    }
    lseek(fd, 0, SEEK_SET);

    //Enfin on écrit tous les paquets dans le fichier.
    paquet * courant = pop_paquet(liste_paq);
    while (courant != NULL) {
        int r = write(fd, courant -> data, strlen(courant -> data));
        if (r < 0) {
            perror("write "); 
            free_liste_paquets(liste_paq);
            close(fd);
            return -1;
        }

        paquet * suiv = courant -> next;
        free_paquet(courant);
        courant = suiv;
    }

    close(fd);

    return 0;
}