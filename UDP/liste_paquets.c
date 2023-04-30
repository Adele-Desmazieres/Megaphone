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
    int len_paq = strlen(paq.data);
    if (len_paq % 2 != 0) len_paq += 1;

    //Taille: 2 pour les 4 octets de id,codereq,numblock et le reste fait la taille du fichier.
    u_int16_t * msg = malloc(sizeof(u_int16_t) * (2 + len_paq/2 ) );
    if (msg == NULL) return NULL;

    //ENTETE

    //CODEREQ | ID
    msg[0] = htons((paq.id << 5) + paq.codereq);

    //NUMBLOC
    msg[1] = htons(paq.numbloc);

    //DATA
    for (int i = 2; i - 2 < strlen(paq.data); i += 2) {
        //On part de 2 pour msg mais de paq.data[i - 2] car on veut partir de 0.
        //char car1 = paq.data[i - 2];
        //char car2 = paq.data[i - 2 +1];

        char car1 = ( i - 2 < strlen(paq.data)) ? paq.data[i - 2] : '\0';
        char car2 = ( i - 2 +1 < strlen(paq.data)) ? paq.data[i - 2 +1] : '\0';

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

    //On prend une taille de 513 car si on a des données de taille 512 on veut ajouter '\0' à la fin
    char tmp[513];
    memset(tmp, '\0', 513);

    int ind_char = 0;
    int ind_mes = 2;

    int n = sizeof(msg) - 2;

    while(ind_char < 512 || ind_mes < n) {
        tmp[ind_char + 1] = (char) (msg[ind_mes] >> 8);
        tmp[ind_char] = (char) (msg[ind_mes]);
        ind_char += 2;
        ind_mes += 1;
    }

    char * data = malloc(strlen(tmp) + 1);
    if (data == NULL) return NULL;
    memset(data, '\0', strlen(tmp) + 1);
    strncpy(data, tmp, strlen(tmp));

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
        paq -> next = liste -> first;
        liste -> first -> prev = paq;

        liste -> first = paq;
    }
    else {
        paquet * courant = liste -> first;
        while (courant -> next != NULL && paq -> numbloc > courant -> numbloc) {
            courant = courant -> next;
        }
        paq -> prev = courant;
        paq -> next = courant -> next;
        if (courant -> next != NULL) paq -> next -> prev = paq;
        courant -> next = paq;
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
    paquet * courant = liste_paq -> first;
    while (courant != NULL) {
        int r = write(fd, courant -> data, strlen(courant -> data));
        if (r < 0) {
            perror("write "); 
            free_liste_paquets(liste_paq);
            close(fd);
            return -1;
        }

        courant = courant -> next;
    }

    free_liste_paquets(liste_paq);
    close(fd);

    return 0;
}