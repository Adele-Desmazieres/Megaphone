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
#include<sys/stat.h>
#include <fcntl.h> 

#include "serveur.h"

#define PORT 2121
#define SIZE_MSG 512
#define TAILLE_MSG_REP 6
//Valeur à part car modifiables à guises
#define PORT_UDP 2121
#define BUF_SIZE_UDP 512

int main(int argc, char **argv) {
    return creation_serveur();
}

base_serveur * base_serveur_constr(user_list * ul, liste_fils * lf, int sockli) {
    base_serveur * ret = malloc(sizeof(base_serveur));
    if (ret == NULL) perror("Erreur malloc objets thread structure\n");

    ret->liste_uti = ul;
    ret->liste_fils = lf;
    ret->socketclient = sockli;

    return ret;
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
    adrserv.sin6_port = htons(PORT);
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

int connexion_udp() {
    //Creation de la socket UDP serveur IPV6.
    int sock = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) goto error;

    struct sockaddr_in6 adrserv;
    memset(&adrserv, 0, sizeof(adrserv));
    adrserv.sin6_family = AF_INET6;
    adrserv.sin6_port = htons(PORT);
    adrserv.sin6_addr = in6addr_any;

    //Ouverture de la socket avec la prise en charge à la fois de IPV4 et de IPV6 dans une socket polymorphe.
    int no = 0;
    int r = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
    if (r == -1) goto error;

    r = bind(sock, (struct sockaddr *) &adrserv, sizeof(adrserv));
    if (r == -1) goto error;

    return sock;

    error:
    close(sock);
    perror("Erreur création du serveur. "); 
    return -1;
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
    msg_client * msg_client = tcp_to_msgclient(sockcli);
    if (msg_client == NULL) {
        free(arg_base_serveur);
        close(sockcli);
        perror("Problème reception message @ communication_client @ serveur.c");
        exit(EXIT_FAILURE);
    }

    int retour = 0;

    switch(msg_client -> codereq){
        //L'utilisateur veut s'inscrire.
        case 1 :
            retour = inscription_utili(msg_client, base_serv -> liste_uti);
            if (retour == -1) envoi_erreur_client(sockcli);
            else {
                msg_serveur to_send = {1, retour, 0, 0};
                envoie_reponse_client(sockcli, to_send);
            } 
            close(sockcli);
            break;
        //L'utilisateur veut poster un billet.
        case 2 :

            retour = poster_billet(msg_client, base_serv -> liste_fils, base_serv -> liste_uti, msg_client -> data);
            if (retour == -1) envoi_erreur_client(sockcli);
            else {
                msg_serveur to_send = {2, msg_client -> id, retour, 0};
                envoie_reponse_client(sockcli, to_send);
            } 
            close(sockcli);
            break;
        //L'utilisateur demande la liste des n derniers billets
        case 3 :
            liste_n_billets(sockcli, base_serv->liste_fils, msg_client); 
            close(sockcli);
            break;
        //l'utilisateur veut s'abonner à un fil.
        case 4 :
            abonner_fil(); 
            close(sockcli);
            break;
        //L'utilisateur veut envoyer un fichier.
        case 5 :
            retour = udp_envoi_port_client(msg_client, base_serv -> liste_fils, base_serv -> liste_uti); 
            if (retour == -1) {
                envoi_erreur_client(sockcli);
                close(sockcli);
            }
            else {
                msg_serveur to_send = {msg_client -> codereq, msg_client -> id, msg_client -> numfil, retour};
                envoie_reponse_client(sockcli, to_send);
                close(sockcli);
                retour = recevoir_donnees_fichier(msg_client, base_serv -> liste_fils, base_serv -> liste_uti, msg_client -> data);
            }
            break;
        //L'utilisateur veut telecharger un fichier.
        case 6 :
            telecharger_fichier(); break;
        default :
            envoi_erreur_client(sockcli); break;
    }

    free(arg_base_serveur);
    free(msg_client);

    return NULL;
}

void envoi_erreur_client(int sockcli) {
    msg_serveur message_erreur = {31, 0, 0, 0};
    uint16_t * msg_reponse = msg_serveur_to_send(message_erreur);

    int snd = send(sockcli, msg_reponse, TAILLE_MSG_REP, 0);
    if (snd <= 0){
        perror("Erreur envoi réponse\n");
    }

    free(msg_reponse);
}

void envoie_reponse_client(int sockcli, msg_serveur reponse_serveur) {
    uint16_t * msg_reponse = msg_serveur_to_send(reponse_serveur);

    int snd = send(sockcli, msg_reponse, TAILLE_MSG_REP, 0);
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
int poster_billet(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * contenu) {
    int num_fil = msg_client -> numfil;

    //Si num_fil vaut 0 alors l'utilisateur cherche à poster sur un nouveau fil avec pseudo et texte.
    if (msg_client -> numfil == 0) {
        char * username = get_name(liste_utili, msg_client -> id);
        
        fil * nouveau_fil = fil_constr(username, contenu);
        if (nouveau_fil == NULL) { perror("Erreur creation de fil\n."); return 1; }
        num_fil = ajouter_fil(liste_fils, nouveau_fil);
    }

    //Sinon on cherche ou il veut poster puis on ajoute le billet à cet endroit.
    else {
        char * username = get_name(liste_utili, msg_client -> id);
        fil * fil_poster = get_fil_id(liste_fils, msg_client -> numfil);
        //Le fil que l'utilisateur a voulu selectionner n'existe pas.
        if (fil_poster == NULL) return -1;
        ajouter_billet(fil_poster, username, contenu);
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
        free(prem_reponse_a_envoyer);

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


int udp_envoi_port_client(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili) {
    int num_fil = msg_client -> numfil;

    //Si le client veut poster sur un fil on regarde si celui ci est valide avant de renvoyer le numéro
    //de port
    if (num_fil != 0) {
        fil * fil_poster = get_fil_id(liste_fils, msg_client -> numfil);
        if (fil_poster == NULL) return -1;
    }

    return PORT_UDP;
}

int recevoir_donnees_fichier(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * file_name) {
    //On créé notre connexion udp ainsi que l'adresse client.
    int sock = connexion_udp();
    if (sock < 0) return -1;

    struct sockaddr_in6 adrclient;
    socklen_t sizeclient = sizeof(adrclient);

    //On créé la liste qui va pouvoir stocker tous les paquets reçus.
    liste_paquets * liste_paq = malloc(sizeof(liste_paquets));
    if (liste_paq == NULL) return -1;
    liste_paq -> first = NULL;

    int taille_msg_udp = sizeof(u_int16_t) * (2 + (512)/2);
    u_int16_t buff[taille_msg_udp];
    int r = taille_msg_udp;

    int nb_paquets = 0;
    int num_dernier_paq = -1;

    //Tant que r vaut la taille d'un msg udp on continue de recevoir des données, 
    //sinon c'est que l'on a récupéré le dernier paquet.
    while(num_dernier_paq == -1 || nb_paquets < num_dernier_paq) {
        memset(buff, 0, taille_msg_udp);
        r = recvfrom(sock, buff, taille_msg_udp, 0, (struct sockaddr *) &adrclient, &sizeclient);
        if (r < 0) {
            perror("recv ");
            free_liste_paquets(liste_paq);
            close(sock);
            return -1;
        }
        paquet * paq = udp_to_paquet(buff);
        if (paq == NULL) {
            perror("paq ");
            free_liste_paquets(liste_paq);
            close(sock);
            return -1;
        }
        push_paquet(liste_paq, paq);
        if (strlen(paq -> data) < 512) num_dernier_paq = paq -> numbloc;
        nb_paquets += 1;
    }

    //On écrit enfin dans le fichier toute la liste des paquets.s
    r = ecrire_dans_fichier_udp("fichier_recu.txt", liste_paq);
    if (r == -1) {
        close(sock);
        return -1;
    }

    close(sock);

    //CONVERTIR NOM DU FICHIER + ESPACE + SA TAILLE
    char taille_nom_fic[5];
    sprintf(taille_nom_fic, "%d", msg_client ->datalen);

    //La taille du texte est composée de la taille du nom du fichier, d'un espace, de la taille
    //du int fichier puis du charactère \0.
    size_t taille_texte_total = strlen(taille_nom_fic) + 1 + strlen(msg_client -> data) + 1;
    char contenu_billet[taille_texte_total + 1];
    memset(contenu_billet, '\0', taille_texte_total + 1);

    //On recopie le nom du fichier, puis on ajoute un espace puis on ajoute la taille du fichier.
    strncpy(msg_client -> data, contenu_billet, strlen(msg_client -> data));
    contenu_billet[ msg_client -> datalen + 1 ] = ' ';
    strncpy(taille_nom_fic, contenu_billet + msg_client -> datalen + 2, strlen(taille_nom_fic));

    poster_billet(msg_client, liste_fils, liste_utili, contenu_billet);
    
    return 0;
}

void telecharger_fichier() {
    
}