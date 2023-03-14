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

#define SIZE_MSG 512

typedef struct base_serveur {
    user_list * liste_utilisateurs;
    liste_fils * liste_fils;
    int * socketclient;
} base_serveur;

base_serveur * base_serveur_constr(user_list * ul, liste_fils * lf, int * sockli){
    base_serveur * ret = malloc(sizeof(base_serveur));
    if (ret == NULL) perror("Erreur malloc objets thread structure\n");

    ret->liste_utilisateurs = ul;
    ret->liste_fils = lf;
    ret->socketclient = sockli;

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

        int sockli = accept(sock, NULL, NULL);
        if (sockli == -1) {
            perror("Probleme socket client\n");
            close(sock);
            continue;
        }
        int * sockcli_malloc = malloc(sizeof(int));
        *sockcli_malloc = sockli;

        base_serveur * base_serv = base_serveur_constr(liste_users, liste_fil, sockcli_malloc);

        //Création du thread et lancement de sa routine
        pthread_t thread;
        if(pthread_create(&thread, NULL, communication_client, base_serv) == -1){
            perror("Echec connexion\n");
            continue;
        }

        //ca peut ptet poser des problemes ici, faudrait attendre que le thread soit termine pour les free
        free(sockcli_malloc);
        free(base_serv);

    }

    free_liste_fils(liste_fil);
    free_userlist(liste_users);

    return 0;
}

/*
    Effectue la communication entre le serveur et le/les clients.
*/
void * communication_client(void * arg_base_serveur) {

    base_serveur * base_serv = ((base_serveur *) arg_base_serveur);
    int sockcli = *(base_serv->socketclient); 
    char BUF[512];
    memset(BUF, '\0', 512);

    int recu = recv(sockcli, BUF, 512, 0);

    if(recu <= 0) {
        close(sockcli);
        return NULL;
    }

    msg_client * msg_recu_traduit = tcp_to_msgclient((uint16_t *)BUF);
    int retour = 0;

    switch(msg_recu_traduit -> codereq){
        //L'utilisateur veut s'inscrire.
        case 0 :
            retour = inscription_utili(msg_recu_traduit, base_serv -> liste_utilisateurs);
            if (retour == -1) envoi_erreur_client(sockcli);
            else {
                msg_serveur to_send = {1, retour, 0, 0};
                envoie_reponse_client(sockcli, to_send);
            }
        //L'utilisateur veut poster un billet.
        case 2 :
            retour = poster_billet(msg_recu_traduit, base_serv -> liste_fils, base_serv -> liste_utilisateurs);
            if (retour == -1) envoi_erreur_client(sockcli);
            else {
                msg_serveur to_send = {2, msg_recu_traduit -> id, retour, 0};
                envoie_reponse_client(sockcli, to_send);
            }
        //L'utilisateur demande la liste des n derniers billets
        case 3 :
            liste_n_billets();
        //l'utilisateur veut s'abonner à un fil.
        case 4 :
            abonner_fil();
        //L'utilisateur veut envoyer un fichier.
        case 5 :
            ajouter_fichier();
        //L'utilisateur veut telecharger un fichier.
        case 6 :
            telecharger_fichier();
        default :
            envoi_erreur_client(sockcli);
    } 

    close(sockcli);
    return NULL;    
}

void envoi_erreur_client(int sockcli) {
    msg_serveur message_erreur = {31, 0, 0, 0};
    uint16_t * msg_reponse = msg_serveur_to_send(message_erreur);

    int snd = send(sockcli, msg_reponse, 512, 0);
    if (snd <= 0){
        perror("Erreur envoi réponse\n");
    }

    free(msg_reponse);
}

void envoie_reponse_client(int sockcli, msg_serveur reponse_serveur) {
    uint16_t * msg_reponse = msg_serveur_to_send(reponse_serveur);

    int snd = send(sockcli, msg_reponse, 512, 0);
    if (snd <= 0){
        perror("Erreur envoi réponse\n");
    }

    free(msg_reponse);
}

//Fonction qui permet à l'utilisateur de s'inscrire. Renvoie l'identifiant de l'utilisateur
//en cas de succès.
int inscription_utili(msg_client * msg_client, user_list * liste_utili) {
    int r = add_user(liste_utili, msg_client -> data);
    if (r == 0) return -1;
    return get_id(liste_utili, msg_client -> data);
}

//Fonction qui permet de poster un billet dans un fil passé en parametre du message. Renvoie le numéro 
//du fil ou le billet a été posté en cas de succès.
int poster_billet(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili) {
    int num_fil = msg_client -> numfil;

    //Si num_fil vaut 0 alors l'utilisateur cherche à poster sur un nouveau fil avec pseudo et texte.
    if (msg_client -> numfil == 0) {
        char * username = get_name(liste_utili, msg_client -> id);
        fil * nouveau_fil = fil_constr(username, msg_client -> data);
        if (nouveau_fil == NULL) { perror("Erreur creation de fil\n."); return 1; }
        num_fil = ajouter_fil(liste_fils, nouveau_fil);
    }

    //Sinon on cherche ou il veut poster puis on ajoute le billet à cet endroit.
    else {
        char * username = get_name(liste_utili, msg_client -> id);
        fil * fil_poster = get_fil_id(liste_fils, msg_client -> numfil);
        //Le fil que l'utilisateur a voulu selectionner n'existe pas.
        if (fil_poster == NULL) return -1;
        ajouter_billet(fil_poster, username, msg_client -> data);
    }

    return num_fil;
}

void liste_n_billets() {
    
}

void abonner_fil() {

}

void ajouter_fichier() {

}

void telecharger_fichier() {
    
}