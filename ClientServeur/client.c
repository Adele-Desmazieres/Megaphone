#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "client.h"

#define SIZE_MSG 512

/*
    Connexion utilisant le protocole IPV4.
*/
int connexion_4() {
    //On créé la socket IPV4.
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adrclient;
    memset(&adrclient, 0, sizeof(adrclient));

    adrclient.sin_family = AF_INET;
    adrclient.sin_port = htons(2121);
    inet_pton(AF_INET, "127.0.0.1", &adrclient.sin_addr);

    //On se connecte au serveur.
    int r = connect(sock, (struct sockaddr *) &adrclient, sizeof(adrclient));
    if (r == -1) goto error;
    else {
        printf("Connexion établie !\n");
        return sock;
    }

    error:
    perror("Erreur connexion au serveur "); 
    close(sock);
    return -1;
}

/*
    Connexion utilisant le protocole IPV6.
*/
int connexion_6() {
    //On créé la socket IPV6.
    int sock = socket(PF_INET6, SOCK_STREAM, 0);
    struct sockaddr_in6 adrclient;
    memset(&adrclient, 0, sizeof(adrclient));

    adrclient.sin6_family = AF_INET6;
    adrclient.sin6_port = htons(2121); // 7777 et "::1" pour se connecter au serv prof sur lulu
    inet_pton(AF_INET6, "fe80:::43ff:fe49:79bf", &adrclient.sin6_addr);

    //On se connecte au serveur.
    int r = connect(sock, (struct sockaddr *) &adrclient, sizeof(adrclient));
    if (r == -1) goto error;
    else {
        printf("Connexion établie !\n");
        return sock;
    }

    error:
    perror("Erreur connexion au serveur "); 
    close(sock);
    return -1;
}
