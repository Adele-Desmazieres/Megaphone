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

#define PORT 2121 // 7777 et "::1" pour se connecter au serv prof sur lulu
#define ADR_IPV4 "127.0.0.1"
#define ADR_IPV6 "fe80:::43ff:fe49:79bf"
#define SIZE_MSG 512

/*
    Connexion TCP utilisant le protocole IPV4.
*/
int connexion_4() {
    //On créé la socket IPV4.
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    struct sockaddr_in adrclient;
    memset(&adrclient, 0, sizeof(adrclient));

    adrclient.sin_family = AF_INET;
    adrclient.sin_port = htons(PORT);
    inet_pton(AF_INET, ADR_IPV4, &adrclient.sin_addr);

    //On se connecte au serveur.
    int r = connect(sock, (struct sockaddr *) &adrclient, sizeof(adrclient));
    if (r == -1) goto error;
    else return sock;

    error:
    perror("Erreur connexion au serveur "); 
    close(sock);
    return -1;
}

/*
    Connexion TCP utilisant le protocole IPV6.
*/
int connexion_6() {
    //On créé la socket IPV6.
    int sock = socket(PF_INET6, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    struct sockaddr_in6 adrclient;
    memset(&adrclient, 0, sizeof(adrclient));

    adrclient.sin6_family = AF_INET6;
    adrclient.sin6_port = htons(PORT); // 7777 et "::1" pour se connecter au serv prof sur lulu
    inet_pton(AF_INET6, ADR_IPV6, &adrclient.sin6_addr);

    //On se connecte au serveur.
    int r = connect(sock, (struct sockaddr *) &adrclient, sizeof(adrclient));
    if (r == -1) goto error;
    else return sock;

    error:
    perror("Erreur connexion au serveur "); 
    close(sock);
    return -1;
}

/*
    Connexion UDP utilisant le protocole IPV4.
*/
int connexion_udp_4(struct sockaddr_in * adrclient, int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    adrclient -> sin_family = AF_INET;
    adrclient -> sin_port = htons(port);
    inet_pton(AF_INET, ADR_IPV4, &adrclient -> sin_addr);

    return sock;
}

/*
    Connexion UDP utilisant le protocole IPV6.
*/
int connexion_udp_6(struct sockaddr_in6 * adrclient, int port) {
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    adrclient -> sin6_family = AF_INET6;
    adrclient -> sin6_port = htons(port);
    inet_pton(AF_INET, ADR_IPV6, &adrclient -> sin6_addr);

    return sock;
}