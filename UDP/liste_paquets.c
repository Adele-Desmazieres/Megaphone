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
#include <time.h>
#include <errno.h>

#include "liste_paquets.h"

#define SIZE_PAQ 512
#define TEMPS_PAQ 100

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

/*
    Renvoie la taille d'un msg par rapport à la taille de paquet en accord avec les uint16.
*/
size_t get_taille_msg_udp(paquet paq) {
    int len_paq = strlen(paq.data);
    if (len_paq % 2 != 0) len_paq += 1;

    //Taille: 2 pour les 4 octets de id,codereq,numblock et le reste fait la taille du fichier.
    return sizeof(u_int16_t) * (2 + len_paq/2 );
}

/*
    Transforme les données d'un paquet paq en message uint16.
*/
u_int16_t * paquet_to_udp(size_t len_paq, paquet paq) {
    u_int16_t * msg = malloc(len_paq);
    if (msg == NULL) return NULL;

    //ENTETE

    //CODEREQ | ID
    msg[0] = htons((paq.id << 5) + paq.codereq);

    //NUMBLOC
    msg[1] = htons(paq.numbloc);

    //DATA
    for (int i = 2; i - 2 < strlen(paq.data); i += 2) {
        char car1 = ( i - 2 < strlen(paq.data)) ? paq.data[i - 2] : '\0';
        char car2 = ( i - 2 +1 < strlen(paq.data)) ? paq.data[i - 2 +1] : '\0';

        msg[(i/2) + 1] = (u_int16_t)(((int)car2  << 8) + (int)car1);
    }

    return msg;
}

/*
    Transforme un message uint16 en un paquet.
*/
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

    //On transforme les char en octet uint16 2 par 2.
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

/*
    Ajoute un paquet dans l'ordre de la liste des paquets en fonction de son numblock.
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

/*
    Libère les données d'un paquet.
*/
void free_paquet(paquet * paq) {
    free(paq -> data);
    free(paq);
}

/*
    Libère la liste des paquets.
*/
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

/*
    Envoie les données d'un fichier à la sock passée en parametre ainsi que l'adrudp et le port.
*/
int envoyer_donnees_fichier(int sock, struct sockaddr_in6 adrudp, int codereq, int port, char * file_name, int directory_client) {
    char * directory_name = get_directory_file(file_name, directory_client);

    FILE * fd = fopen(directory_name, "r");
    if (fd == NULL) { perror("fopen"); return -1; }

    //On récupère la taille du fichier.
    fseek(fd, 0, SEEK_END);
    int file_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    //On prend + 1 pour prendre en compte le symbole '\0' pour strlen
    char read_buff[SIZE_PAQ + 1];
    memset(read_buff, '\0', SIZE_PAQ + 1);

    int numbloc = 0;
    socklen_t len = sizeof(adrudp);
    int r = fread(read_buff, SIZE_PAQ, 1, fd);

    //On lit les données de notre fichier, transforme en msg uint16 et on envoie.
    while (strlen(read_buff) > 0) {
        //On attends un peu de temps avant l'envoi de chaque paquets
        struct timespec time_spec;
        time_spec.tv_sec = TEMPS_PAQ / 1000;
        time_spec.tv_nsec = (TEMPS_PAQ % 1000) * 1000000;
        nanosleep(&time_spec, &time_spec);

        paquet paq = {5, codereq, numbloc, read_buff};
        size_t len_paq = get_taille_msg_udp(paq);
        u_int16_t * msg = paquet_to_udp(len_paq, paq);
        r = sendto(sock, msg, len_paq, 0, (struct sockaddr *)&adrudp, len);
        if (r < 0){ perror("sendto "); return -1;}

        memset(read_buff, '\0', SIZE_PAQ + 1);
        r = fread(read_buff, SIZE_PAQ, 1, fd);
        numbloc += 1;
        free(msg);
    }
    if (ferror(fd))
        perror("Erreur lecture ");

    //Si la taille du fichier est divisible par 512 on envoie un paquet vide.
    if (file_size % 512 == 0) {
        memset(read_buff, 0, SIZE_PAQ + 1);
        paquet paq = {5, codereq, numbloc, read_buff};
        size_t len_paq = get_taille_msg_udp(paq);
        u_int16_t * msg = paquet_to_udp(len_paq, paq);
        r = sendto(sock, msg, len_paq, 0, (struct sockaddr *)&adrudp, len);
        free(msg);
    }

    free(directory_name);
    fclose(fd);
    close(sock);
    
    return 0;
}

/*
    Recoit les données d'un fichier à la sock passée en parametre.
*/
int recevoir_donnees_fichier(int sock, char * file_name, int directory_client) {
    //On créé la liste qui va pouvoir stocker tous les paquets reçus.
    liste_paquets * liste_paq = malloc(sizeof(liste_paquets));
    if (liste_paq == NULL) return -1;
    liste_paq -> first = NULL;

    int taille_msg_udp = sizeof(u_int16_t) * (2 + (512)/2);
    u_int16_t buff[taille_msg_udp];
    memset(buff, '\0', sizeof(buff));

    int r = taille_msg_udp;

    int nb_paquets = 0;
    int num_dernier_paq = -1;

    struct sockaddr_in6 recv_adr;
    socklen_t len = sizeof(recv_adr);

    //Tant que r vaut la taille d'un msg udp on continue de recevoir des données, 
    //sinon c'est que l'on a récupéré le dernier paquet.
    while(num_dernier_paq == -1 || nb_paquets < num_dernier_paq) {
        memset(buff, 0, taille_msg_udp);
        r = recvfrom(sock, buff, taille_msg_udp, 0, (struct sockaddr *)&recv_adr, &len);
        if (r < 0) { perror("recv "); free_liste_paquets(liste_paq); return -1; }

        paquet * paq = udp_to_paquet(buff);
        if (paq == NULL) { perror("paq "); free_liste_paquets(liste_paq); return -1; }
        push_paquet(liste_paq, paq);

        if (strlen(paq -> data) < 512) num_dernier_paq = paq -> numbloc;
        nb_paquets += 1;
    }

    //On écrit enfin dans le fichier toute la liste des paquets.
    r = ecrire_dans_fichier_udp(file_name, liste_paq, directory_client);
    if (r == -1) return -1;

    return 0;
}

/*
    Ecrit les données de tous les paquets dans un fichier.
*/
int ecrire_dans_fichier_udp(char * file_name, liste_paquets * liste_paq, int directory_client) {
    char * directory_name = get_directory_file(file_name, directory_client);

    //On ouvre le fichier et le créé si besoin.
    FILE * fd = fopen(directory_name, "w+");
    if (fd == NULL) {
        perror("fopen ");
        free_liste_paquets(liste_paq);
        return -1;
    }
    fseek(fd, 0, SEEK_SET);

    //Enfin on écrit tous les paquets dans le fichier.
    paquet * courant = liste_paq -> first;
    while (courant != NULL) {
        int r = fwrite(courant -> data, strlen(courant -> data), 1, fd);
        if (r < 0) {
            perror("fwrite "); 
            free_liste_paquets(liste_paq);
            fclose(fd);
            return -1;
        }

        courant = courant -> next;
    }

    free(directory_name);
    free_liste_paquets(liste_paq);
    fclose(fd);

    return 0;
}

char * get_directory_file(char * file_name, int directory_client) { 
    char * directory_name;
    if (directory_client == 1) {
        directory_name = malloc(strlen("FicClient/") + strlen(file_name) + 1);
        directory_name[strlen("cClient/") + strlen(file_name)] = '\0';
        strcpy(directory_name, "FicClient/");
        strcat(directory_name, file_name);
    }
    else {
        directory_name = malloc(strlen("FicServ/") + strlen(file_name) + 1);
        directory_name[strlen("FicServ/") + strlen(file_name)] = '\0';
        strcpy(directory_name, "FicServ/");
        strcat(directory_name, file_name);
    }

    return directory_name;
}