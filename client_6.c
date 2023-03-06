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

int connexion() {
    //On créé la socket IPV6.
    int sock = socket(PF_INET6, SOCK_STREAM, 0);
    struct sockaddr_in6 adrclient;
    memset(&adrclient, 0, sizeof(adrclient));

    adrclient.sin6_family = AF_INET6;
    adrclient.sin6_port = htons(2121);
    inet_pton(AF_INET6, "fe80:::43ff:fe49:79bf", &adrclient.sin6_addr);

    //On se connecte au serveur.
    int r = connect(sock, (struct sockaddr *) &adrclient, sizeof(adrclient));
    if (r == -1) goto error;
    else {
        printf("Connexion établie !\n");
        return communication(sock);
    }

    error:
    perror("Erreur connexion au serveur.\n"); 
    return EXIT_FAILURE;
}

int communication(int sock) {
    char buf[SIZE_MSG];
    memset(buf, 0, SIZE_MSG);
    sprintf(buf, "Test message client");
    int ecrit = send(sock, buf, strlen(buf), 0);

    return 0;
}