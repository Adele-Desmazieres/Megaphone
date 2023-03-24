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
    int socketclient;
} base_serveur;

base_serveur * base_serveur_constr(user_list * ul, liste_fils * lf, int sockli){
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

        base_serveur * base_serv = base_serveur_constr(liste_users, liste_fil, sockli);

        //Création du thread et lancement de sa routine
        pthread_t thread;
        if(pthread_create(&thread, NULL, communication_client, base_serv) == -1){
            perror("Echec connexion\n");
            continue;
        }

        //ca peut ptet poser des problemes ici, faudrait attendre que le thread soit termine pour les free
        //free(base_serv);

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
    int sockcli = (base_serv->socketclient); 

    //tcp_to_msgclient effectue les recv qui correspondent au premier message reçu
    msg_client * msg_recu_traduit = tcp_to_msgclient(sockcli);
    if (msg_recu_traduit == NULL){
        close(sockcli);
        perror("Problème reception message @ communication_client @ serveur.c");
        exit(EXIT_FAILURE);
    }

    //printf("Pseudo : %s\n", msg_recu_traduit->data);

    int retour = 0;

    switch(msg_recu_traduit -> codereq){
        //L'utilisateur veut s'inscrire.
        case 1 :

            retour = inscription_utili(msg_recu_traduit, base_serv -> liste_utilisateurs);
            if (retour == -1) envoi_erreur_client(sockcli);
            else {
                msg_serveur to_send = {1, retour, 0, 0};
                envoie_reponse_client(sockcli, to_send);
            } break;
        //L'utilisateur veut poster un billet.
        case 2 :
            retour = poster_billet(msg_recu_traduit, base_serv -> liste_fils, base_serv -> liste_utilisateurs);
            if (retour == -1) envoi_erreur_client(sockcli);
            else {
                msg_serveur to_send = {2, msg_recu_traduit -> id, retour, 0};
                envoie_reponse_client(sockcli, to_send);
            } break;
        //L'utilisateur demande la liste des n derniers billets
        case 3 :
            liste_n_billets(sockcli, base_serv->liste_fils, msg_recu_traduit); break;
        //l'utilisateur veut s'abonner à un fil.
        case 4 :
            abonner_fil(); break;
        //L'utilisateur veut envoyer un fichier.
        case 5 :
            ajouter_fichier(); break;
        //L'utilisateur veut telecharger un fichier.
        case 6 :
            telecharger_fichier(); break;
        default :
            envoi_erreur_client(sockcli); break;
    } 

    close(sockcli);
    free(arg_base_serveur);
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

void liste_n_billets(int sockcli, liste_fils * liste_fils, msg_client * msg_client) {
    
    //On envoie la première réponse

    int numfil = (msg_client->numfil > 0) ? msg_client -> numfil : liste_fils -> nb_de_fils ;
    int nb;

    //SI UN SEUL FIL
    if(numfil != 0){

        nb = (msg_client->nb > get_fil_id (liste_fils, numfil)->nb_de_msg || msg_client->nb == 0) ? ( get_fil_id(liste_fils , numfil) -> nb_de_msg ) : msg_client->nb ;

        //On envoie le premier message annoncant le nombre de messages qui vont suivre
        msg_serveur prem_reponse = {msg_client->codereq, msg_client->id, numfil, nb};
        uint16_t * prem_reponse_a_envoyer = msg_serveur_to_send(prem_reponse);

        int snd = send(sockcli, prem_reponse_a_envoyer, 512, 0);
        if (snd <= 0){
            perror("Erreur envoi réponse\n");
        }

        //On récupère le fil choisi
        fil * fil = get_fil_id(liste_fils, numfil);
        int nb_billets = (nb > fil->nb_de_msg) ? fil->nb_de_msg : nb;

        //On parcourt ses n derniers billets
        billet * billets_a_envoyer = get_n_derniers_billets(fil, nb_billets);
        for(int i = 0; i < nb_billets; i++){

            billet actuel = billets_a_envoyer[i];
            msg_billet_envoi a_envoyer = { numfil, strlen(actuel.texte), fil->premier_msg->auteur, actuel.auteur, actuel.texte};
            uint16_t * msg_a_envoyer = msg_billet_to_send(a_envoyer);

            //On envoie le billet actuel
            snd = send(sockcli, msg_a_envoyer, 512, 0);
            if (snd <= 0){
                perror("Erreur envoi réponse\n");
            }

            free(msg_a_envoyer);

        }

        free(billets_a_envoyer);

        return;

    }

    //SI TOUS LES FILS

    //NOMBRE REEL DE BILLETS A ENVOYER PAR FIL
    int nbr_billets[liste_fils-> nb_de_fils];

    //On parcourt la liste des fils pour récupérer le nombre réel de billets à envoyer selon le fil
    fil * tmp = liste_fils->premier_fil;
    for(int i = 0; i < liste_fils->nb_de_fils; i++, tmp = tmp->suiv){
        nbr_billets[i] = (msg_client -> nb > tmp->nb_de_msg) ? tmp->nb_de_msg : msg_client -> nb;
        nb += nbr_billets[i];
    }

    //On envoie le premier message annoncant le nombre de messages qui vont suivre
    msg_serveur prem_reponse = {msg_client->codereq, msg_client->id, numfil, nb};
    uint16_t * prem_reponse_a_envoyer = msg_serveur_to_send(prem_reponse);

    int snd = send(sockcli, prem_reponse_a_envoyer, 512, 0);
    if (snd <= 0){
        perror("Erreur envoi réponse\n");
    }

    free(prem_reponse_a_envoyer);

    //On parcourt la liste des fils
    tmp = liste_fils->premier_fil;
    for(int i = 0; i < liste_fils->nb_de_fils; i++, tmp = tmp->suiv){

        //on récupère la liste des billets de ce fil actuel
        billet * billets_pour_fil_actuel = get_n_derniers_billets(tmp, nbr_billets[i]);
        for(int j = 0; j < nbr_billets[i]; j++){

            //On transforme le billet actuel en message à envoyer
            billet actuel = billets_pour_fil_actuel[j];
            msg_billet_envoi a_envoyer = { tmp->id, strlen(actuel.texte), tmp->premier_msg->auteur, actuel.auteur, actuel.texte};
            uint16_t * msg_a_envoyer = msg_billet_to_send(a_envoyer);

            //On l'envoie
            snd = send(sockcli, msg_a_envoyer, 512, 0);
            if (snd <= 0){
                perror("Erreur envoi réponse\n");
            }

            free(msg_a_envoyer);

        }

        free(billets_pour_fil_actuel);


    }


}

void abonner_fil() {

}

void ajouter_fichier() {

}

void telecharger_fichier() {
    
}