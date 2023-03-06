#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>

#include "serveur.h"

#define SIZE_MSG 512

int main(int argc, char **argv) {
    return creation_serveur();
}

/*
    Créé le serveur, renvoie 1 si raté et sinon appelle les autres fonctions de communication.
*/
int creation_serveur() {
    //Creation de la socket serveur IPV6.
    int sock = socket(PF_INET6, SOCK_STREAM, 0);
    if (sock < 0) goto error;

    struct sockaddr_in6 adrserv;
    memset(&adrserv, 0, sizeof(adrserv));
    adrserv.sin6_family = AF_INET6;
    adrserv.sin6_port = htons(2121);
    adrserv.sin6_addr = in6addr_any;

    //Ouverture de la socket avec la prise en charge à la fois de IPV4 et de IPV6 dans une socket polymorphe.
    int no = 0;
    int r = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
    if (r == -1) goto error;

    r = bind(sock, (struct sockaddr *) &adrserv, sizeof(adrserv));
    if (r == -1) goto error;
    
    r = listen(sock, 0);
    if (r == -1) goto error;

    return accepter_clients(sock);

    error:
    close(sock);
    perror("Erreur création du serveur. "); 
    return EXIT_FAILURE;
}

/*
    Accepte les clients avec en parametre la socket serveur.
*/
int accepter_clients(int sock) {
    //On écoute chaque clients
    int sockclient = accept(sock, NULL, NULL);
    if (sockclient == -1) {
        perror("Probleme socket client\n");
        close(sock);
        return 1;
    }

    //Creation de la structure pour stocker l'adresse client.
    struct sockaddr_in adrclient;
    memset(&adrclient, 0, sizeof(adrclient));
    socklen_t size = sizeof(adrclient);

    //Affichage d'un message avec l'adresse client.
    char addr_buf[INET_ADDRSTRLEN];
    memset(addr_buf, 0, sizeof(addr_buf));
    inet_ntop(AF_INET6, &(adrclient.sin_addr), addr_buf, sizeof(addr_buf));
    printf("Client connecte : %s %d\n", addr_buf, ntohs(adrclient.sin_port));

    int retour = communication_client(sockclient);
    close(sock);

    return retour;
}

/*
    Effectue la boucle de communication entre le serveur et le/les clients.
*/
int communication_client(int sockclient) {
    char BUF[512];
    memset(BUF, '\0', 512);

    //Recevoir/Envoyer les messages.

    close(sockclient);

    return 0;
}

/*
    Renvoie une instance de msg_client en fonction de codereq.
*/
msg_client reponse_tcp(char * buf) {
    msg_client reponse;
    
    return reponse;
}