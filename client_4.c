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

int connexion() {
    //On créé la socket IPV4.
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adrclient;
    memset(&adrclient, 0, sizeof(adrclient));

    adrclient.sin_family = AF_INET;
    adrclient.sin_port = htons(2121);
    inet_pton(AF_INET, "192.168.70.73", &adrclient);

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