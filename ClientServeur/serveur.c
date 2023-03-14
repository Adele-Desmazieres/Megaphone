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

#include "serveur.h"
#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"
#include "bdd_serveur.h"

#define SIZE_MSG 512

typedef struct objets_a_passer_au_thread_dans_une_structure {
    user_list * liste_utilisateurs;
    liste_fils * liste_fils;
    int * socketclient;
} objets_a_passer_au_thread_dans_une_structure;

objets_a_passer_au_thread_dans_une_structure * objet_thread_constr(user_list * ul, liste_fils * lf, int * sockclient){
    objets_a_passer_au_thread_dans_une_structure * ret = malloc(sizeof(objets_a_passer_au_thread_dans_une_structure));
    if (ret == NULL) perror("Erreur malloc objets thread structure\n");

    ret->liste_utilisateurs = ul;
    ret->liste_fils = lf;
    ret->socketclient = sockclient;

    return ret;
}

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

    int serveur_running = 1;
    
    user_list * liste_users = user_list_constr();
    liste_fils * liste_fil = liste_fils_constr();

    //Boucle principale
    while(serveur_running){

        int sockclient = accept(sock, NULL, NULL);
        if (sockclient == -1) {
            perror("Probleme socket client\n");
            close(sock);
            continue;
        }
        int * sockcli_malloc = malloc(sizeof(int));
        *sockcli_malloc = sockclient;

        objets_a_passer_au_thread_dans_une_structure * struc = objet_thread_constr(liste_users, liste_fil, sockcli_malloc);

        //Création du thread et lancement de sa routine
        pthread_t thread;
        if(pthread_create(&thread, NULL, communication_client, struc) == -1){
            perror("Echec connexion\n");
            continue;
        }


        //ca peut ptet poser des problemes ici, faudrait attendre que le thread soit termine pour les free
        free(sockcli_malloc);
        free(struc);

    }

    free_liste_fils(liste_fil);
    free_userlist(liste_users);

    return 0;
}

/*
    Effectue la communication entre le serveur et le/les clients.
*/
void * communication_client(void * arg) {

    objets_a_passer_au_thread_dans_une_structure * obj = ((objets_a_passer_au_thread_dans_une_structure *) arg);
    int sockcli = *(obj->socketclient); 
    char BUF[512];
    memset(BUF, '\0', 512);

    int recu = recv(sockcli, BUF, 512, 0);

    if(recu <= 0){
        close(sockcli);
        return NULL;
    }

    msg_client * msg_recu_traduit = tcp_to_msgclient((uint16_t *)BUF);

    switch(msg_recu_traduit->codereq){

        //... TODO TO DO : traiter les autres demandes

        default :

            msg_serveur * to_send = msg_serveur_constr(31,0,0,0);
            uint16_t * msg_reponse = msg_serveur_to_send(*to_send);
            int snd = send(sockcli, msg_reponse, 512, 0);
            if (snd <= 0){
                perror("Erreur envoi réponse\n");
                return NULL;
            }

            free(to_send); free(msg_reponse);

    } 

    close(sockcli);
    return NULL;    
}
