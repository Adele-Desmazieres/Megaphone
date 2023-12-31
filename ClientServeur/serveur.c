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
#include "../UDP/msg_multicast.h"
#include <sys/select.h>

#include "serveur.h"

#define PORT 2121
#define SIZE_MSG 512
#define TAILLE_MSG_REP 6
//Valeur à part car modifiables à guises
#define PORT_UDP 2121
#define BUF_SIZE_UDP 512
#define NOTIF_WAIT_TIME 20

char * last_used_multicast_ip = FIRST_MULTICAST_IP;
int * messages_for_notif = 0;
pthread_mutex_t ver_notif_count = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t ver_lu = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ver_lf = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ver_fic = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
    last_used_multicast_ip = malloc(40);
    memcpy(last_used_multicast_ip, FIRST_MULTICAST_IP, 40);
    return creation_serveur();
}

/*
    Constructeur pour la base de donnée serveur
*/
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

    //Permission de réutilisation de l'adresse
    int ok = 1;
    r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok));
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

int connexion_udp(int port) {
    //Creation de la socket UDP serveur IPV6.
    int sock = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) goto error;

    struct sockaddr_in6 adrserv;
    memset(&adrserv, 0, sizeof(adrserv));
    adrserv.sin6_family = AF_INET6;
    adrserv.sin6_addr = in6addr_any;
    adrserv.sin6_port = htons(port);

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

    pthread_t thread_envoi_notifs;
    if(pthread_create(&thread_envoi_notifs, NULL, gestion_notifications, liste_fil) == -1){
            perror("Echec création thread cote serveur\n");
            return -1;
    }

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
    Thread pour gérer l'envoi des notifications multicast
*/
void * gestion_notifications(void * arg){

    liste_fils * lf = (liste_fils *)arg;

    while(1){

        //On attend àchaque notif pour ne pas en envoyer trop souvent
        sleep(NOTIF_WAIT_TIME);
        pthread_mutex_lock(&ver_notif_count);
        pthread_mutex_lock(&ver_lf);

        //Si il n'y a pas de nouveau message, alors il est inutile de parcourir la liste de fils
        if(messages_for_notif == 0){
            pthread_mutex_unlock(&ver_lf);
            pthread_mutex_unlock(&ver_notif_count);
            continue;
        }
        pthread_mutex_unlock(&ver_notif_count);

        //Sinon, on parcourt chaque fil et on envoie les messages marqués comme nouveaux
        for(fil * tmp = lf->premier_fil; tmp != NULL; tmp = tmp->suiv){

            for (billet * tmp_bil = tmp->premier_msg; tmp_bil != NULL; tmp_bil = tmp_bil->suiv){

                if(tmp_bil->is_new){

                    msg_notif to_snd = { .codereq = 4, .id = 0, .numfil = tmp->id+1, .pseudo = tmp_bil->auteur, .data = tmp_bil->texte};
                    u_int16_t * buf = msg_notif_to_udp(to_snd);

                    if ((sendto(tmp->multicast_sockfd, buf, SIZE_MSG_NOTIF, 0, (struct sockaddr *)tmp->sockopt, sizeof(struct sockaddr_in6))) < 0){
                        perror("Erreur notification @ poster_billet \n");
                    }

                    free(buf);
                    //On oublie pas d'enlever le marqueur nouveau au message envoyé
                    tmp_bil->is_new = 0;
                }

            }

        }
        pthread_mutex_unlock(&ver_lf);

    }

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
            //Ce point virgule est nécessaire apparemment
            ; fil * f = NULL;
            if ((f = get_fil_id(base_serv->liste_fils, msg_client->numfil)) == NULL){
                envoi_erreur_client(sockcli);
            } else {
                abonner_fil(f, msg_client, sockcli, base_serv); 
            }
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
                retour = recevoir_donnees_fichier_serveur(msg_client, base_serv -> liste_fils, base_serv -> liste_uti, msg_client -> data);
            }
            break;
        //L'utilisateur veut telecharger un fichier.
        case 6 :
            retour = fichier_existe_bdd(msg_client, base_serv -> liste_fils);
            if (retour == -1) {
                envoi_erreur_client(sockcli);
                close(sockcli);
            }
            else {
                msg_serveur to_send = {msg_client -> codereq, msg_client -> id, msg_client -> numfil, retour};
                envoie_reponse_client(sockcli, to_send);
                close(sockcli);

                retour = envoyer_donnees_fichier_serveur(msg_client -> nb, msg_client -> data);
            }
            break;
        default :
            envoi_erreur_client(sockcli); break;
    }

    free(arg_base_serveur);
    free(msg_client);

    return NULL;
}

/*
    Envoie un message d'erreur avec pour code 31 au client.
*/
void envoi_erreur_client(int sockcli) {
    msg_serveur message_erreur = {31, 0, 0, 0};
    uint16_t * msg_reponse = msg_serveur_to_send(message_erreur);

    int snd = send(sockcli, msg_reponse, TAILLE_MSG_REP, 0);
    if (snd <= 0){
        perror("Erreur envoi réponse\n");
    }

    free(msg_reponse);
}

/*
    Transforme un objet msg_serveur en uint16 puis l'envoie au client.
*/
void envoie_reponse_client(int sockcli, msg_serveur reponse_serveur) {
    uint16_t * msg_reponse = msg_serveur_to_send(reponse_serveur);

    int snd = send(sockcli, msg_reponse, TAILLE_MSG_REP, 0);
    if (snd <= 0){
        perror("Erreur envoi réponse\n");
    }

    free(msg_reponse);
}

/*
    Permet à l'utilisateur de s'inscrire. Renvoie l'identifiant de l'utilisateur en cas de succès.
*/
int inscription_utili(msg_client * msg_client, user_list * liste_utili) {

    pthread_mutex_lock(&ver_lu);
    int r = add_user(liste_utili, msg_client -> data);
    if (r == 0) r = -1;
    else r = get_id(liste_utili, msg_client -> data);
    pthread_mutex_unlock(&ver_lu);
    return r;
}

/*
    Permet de poster un billet dans un fil passé en parametre du message. Renvoie le numéro 
    du fil ou le billet a été posté en cas de succès.
*/
int poster_billet(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * contenu) {
    int num_fil = msg_client -> numfil;

    //Si num_fil vaut 0 alors l'utilisateur cherche à poster sur un nouveau fil avec pseudo et texte.
    if (msg_client -> numfil == 0) {

        pthread_mutex_lock(&ver_lu);
        char * username = get_name(liste_utili, msg_client -> id);
        pthread_mutex_unlock(&ver_lu);

        
        fil * nouveau_fil = fil_constr(username, contenu);
        if (nouveau_fil == NULL) { perror("Erreur creation de fil\n."); return 1; }
        pthread_mutex_lock(&ver_lf);
        num_fil = ajouter_fil(liste_fils, nouveau_fil);
        pthread_mutex_unlock(&ver_lf);

    }

    //Sinon on cherche ou il veut poster puis on ajoute le billet à cet endroit.
    else {

        pthread_mutex_lock(&ver_lu);
        char * username = get_name(liste_utili, msg_client -> id);
        pthread_mutex_unlock(&ver_lu);

        pthread_mutex_lock(&ver_lf);
        fil * fil_poster = get_fil_id(liste_fils, msg_client -> numfil);
        //Le fil que l'utilisateur a voulu selectionner n'existe pas.
        if (fil_poster == NULL){
            pthread_mutex_unlock(&ver_lf);
            return -1;  
        } 
        ajouter_billet(fil_poster, username, contenu, fil_poster->is_multicast);

        //Pour le multicast, on incrémente le nombre de message à envoyer à la prochaine notification
        if (fil_poster->is_multicast){

            pthread_mutex_lock(&ver_notif_count);
            messages_for_notif++;
            pthread_mutex_unlock(&ver_notif_count);

        }
        pthread_mutex_unlock(&ver_lf);
    }

    return num_fil;
}

/*
    Renvoie la liste des n billets en fonctions des options de l'utilisateur.
*/
void liste_n_billets(int sockcli, liste_fils * liste_fils, msg_client * msg_client) {
    
    //On envoie la première réponse

    int numfil = (msg_client->numfil > 0) ? msg_client -> numfil : liste_fils -> nb_de_fils ;
    int nb;

    //SI UN SEUL FIL
    if(msg_client->numfil != 0){

        pthread_mutex_lock(&ver_lf);
        nb = (msg_client->nb > get_fil_id (liste_fils, numfil)->nb_de_msg || msg_client->nb == 0) ? ( get_fil_id(liste_fils , numfil) -> nb_de_msg ) : msg_client->nb ;
        pthread_mutex_unlock(&ver_lf);

        //On envoie le premier message annoncant le nombre de messages qui vont suivre
        msg_serveur prem_reponse = {msg_client->codereq, msg_client->id, numfil, nb};
        uint16_t * prem_reponse_a_envoyer = msg_serveur_to_send(prem_reponse);

        int snd = send(sockcli, prem_reponse_a_envoyer, 6, 0);
        if (snd <= 0){
            perror("Erreur envoi réponse\n");
        }
        free(prem_reponse_a_envoyer);

        //On récupère le fil choisi

        pthread_mutex_lock(&ver_lf);
        fil * fil = get_fil_id(liste_fils, numfil);
        int nb_billets = (nb > fil->nb_de_msg) ? fil->nb_de_msg : nb;

        //On parcourt ses n derniers billets
        billet * billets_a_envoyer = get_n_derniers_billets(fil, nb_billets);
        for(int i = 0; i < nb_billets; i++){

            billet actuel = billets_a_envoyer[i];
            msg_billet_envoi a_envoyer = { numfil, strlen(actuel.texte), fil->premier_msg->auteur, actuel.auteur, actuel.texte};
            uint16_t * msg_a_envoyer = msg_billet_to_send(a_envoyer);

            //On envoie le billet actuel
            snd = send(sockcli, msg_a_envoyer, 24 + (a_envoyer.datalen - 1) , 0);
            if (snd <= 0){
                perror("Erreur envoi réponse\n");
            }

            free(msg_a_envoyer);

        }

        free(billets_a_envoyer);
        pthread_mutex_unlock(&ver_lf);

        return;

    }

    //SI TOUS LES FILS

    pthread_mutex_lock(&ver_lf);
    //NOMBRE REEL DE BILLETS A ENVOYER PAR FIL
    int nbr_billets[liste_fils-> nb_de_fils];

    //On parcourt la liste des fils pour récupérer le nombre réel de billets à envoyer selon le fil
    nb = 0;
    fil * tmp = liste_fils->premier_fil;
    for(int i = 0; i < liste_fils->nb_de_fils; i++, tmp = tmp->suiv){
        if(msg_client->nb == 0){
            nbr_billets[i] = tmp->nb_de_msg;
        } else {
            nbr_billets[i] = (msg_client -> nb > tmp->nb_de_msg) ? tmp->nb_de_msg : msg_client -> nb;
        }
        nb += nbr_billets[i];
    }

    //On envoie le premier message annoncant le nombre de messages qui vont suivre
    msg_serveur prem_reponse = {msg_client->codereq, msg_client->id, numfil, nb};
    uint16_t * prem_reponse_a_envoyer = msg_serveur_to_send(prem_reponse);

    int snd = send(sockcli, prem_reponse_a_envoyer, 6, 0);
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
            msg_billet_envoi a_envoyer = { tmp->id+1, strlen(actuel.texte), tmp->premier_msg->auteur, actuel.auteur, actuel.texte};
            uint16_t * msg_a_envoyer = msg_billet_to_send(a_envoyer);

            //On l'envoie
            snd = send(sockcli, msg_a_envoyer, 24 + (a_envoyer.datalen - 1), 0);
            if (snd <= 0){
                perror("Erreur envoi réponse\n");
            }

            free(msg_a_envoyer);
        }

        free(billets_pour_fil_actuel);
    }
    pthread_mutex_unlock(&ver_lf);

}

/*
    Abonne un utilisateur à un fil et lui envoie des notifications lorsqu'il y a un nouveau post.
*/
void abonner_fil(fil * f, msg_client * msg_client, int sockcli, base_serveur * bs) {

    pthread_mutex_lock(&ver_lf);
    if(!f->is_multicast){
        //ACTIVER LE MULTICAST POUR F

        f->multicast_sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (f->multicast_sockfd < 0){
            perror("SOCK @ abonner_fil \n");
        }

        //On incrémente l'ip pour en avoir une différente pour chaque fil en multicast
        f->multicast_addr = malloc(40);
        char * new_ip = incr_ip(last_used_multicast_ip);
        memcpy(last_used_multicast_ip, new_ip, 40);
        memcpy(f->multicast_addr, new_ip, 40);
        free(new_ip);

        int ifindex = 0;
        if(setsockopt(f->multicast_sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex))){
            perror("erreur option socket multicast \n");
        }

        int reuse = 1;
        if(setsockopt(f->multicast_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
            perror("erreur option socket multicast \n");
        }

        struct sockaddr_in6 grsock = {0};
        grsock.sin6_family = AF_INET6;
        inet_pton(AF_INET6, f->multicast_addr, &grsock.sin6_addr);
        grsock.sin6_port = htons(PORT_MULTICAST);


        f->sockopt = malloc(sizeof(struct sockaddr_in6));
        memcpy(f->sockopt, &grsock, sizeof(grsock));

        f->is_multicast = 1;

    }
    pthread_mutex_unlock(&ver_lf);
    msg_demande_abo reponse = { .codereq = msg_client->codereq, .id = msg_client->id, .nb = PORT_MULTICAST, .numfil = msg_client->numfil, .ip = f->multicast_addr};
    u_int16_t * to_snd = msg_abo_to_tcp(reponse);

    if ( (send(sockcli, to_snd, SIZE_MSG_ABO, 0)) < 0){
        perror("Erreur send abonnement\n ");
    }

    free(to_snd);
}


/*
    Envoie le port du serveur si le fil que l'utilisateur a selectionné existe bien.
*/
int udp_envoi_port_client(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili) {

    pthread_mutex_lock(&ver_lf);
    int num_fil = msg_client -> numfil;

    //Si le client veut poster sur un fil on regarde si celui ci est valide avant de renvoyer le numéro
    //de port
    int r = PORT_UDP;
    if (num_fil != 0) {
        fil * fil_poster = get_fil_id(liste_fils, msg_client -> numfil);
        if (fil_poster == NULL) r = -1;
    }
    pthread_mutex_unlock(&ver_lf);

    return r;
}

/*
    Recevoit les données du client pour les écrire dans un fichier serveur et créer un billet.
*/
int recevoir_donnees_fichier_serveur(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * file_name) {
    int sockudp = connexion_udp(PORT);
    if (sockudp < 0) { perror("sock "); return 1; }

    int r = recevoir_donnees_fichier(sockudp, file_name, 0);
    close(sockudp);
    if (r == -1) return -1;

    //On converti la taille du nom du fichier en string.
    char taille_nom_fic[5];
    sprintf(taille_nom_fic, "%d", msg_client -> datalen);

    size_t sz_of_name = strlen(file_name);
    size_t sz_file_length = strlen(taille_nom_fic);

    //La taille du texte est composée de la taille du int fichier puis un espace puis la taille du string
    //puis du charactère \0.
    size_t total_size = sz_of_name + 1 + sz_file_length + 1;    
    char contenu_billet[total_size];
    memset(contenu_billet, '\0', total_size);

    //On recopie le nom du fichier puis sa taille
    strncpy(contenu_billet, file_name, sz_of_name);
    contenu_billet[sz_of_name] = ' ';
    strncpy(contenu_billet + sz_of_name + 1, taille_nom_fic, sz_file_length);

    poster_billet(msg_client, liste_fils, liste_utili, contenu_billet);
    
    return 0;
}

/*
    Regarde si le fichier existe bien à la fois dans la liste des fils mais aussi physiquement.
*/
int fichier_existe_bdd(msg_client * msg_client, liste_fils * liste_fils) {
    //Si num_fil vaut 0 alors on renvoie directement -1.
    if (msg_client -> numfil == 0) return -1;

    //Sinon on cherche le fil et on cherche le fichier.
    else {

        pthread_mutex_lock(&ver_lf);
        fil * fil_fichier = get_fil_id(liste_fils, msg_client -> numfil);
        //Le fil que l'utilisateur a voulu selectionner n'existe pas.
        if (fil_fichier == NULL) return -1;

        //On vérifie que le fichier existe bien dans le fil que l'on a demandé.
        int r = does_file_exist_fil(fil_fichier, msg_client -> data);
        
        if (r == -1) { pthread_mutex_unlock(&ver_lf); return -1; }
        pthread_mutex_unlock(&ver_lf);

        char * directory_name = get_directory_file(msg_client -> data, 1);

        //On vérifie que ce fichier existe bien.
        FILE * fd = fopen(directory_name, "r");
        if (fd == NULL) {
            printf("Le fichier que vous avez entré n'est pas trouvable.\n");
            free(directory_name);
            return -1;
        }
        free(directory_name);
    }

    return 0;
}

/*
    Envoie les données du fichier en parametre vers le numéro de port du client qui l'a demandé.
*/
int envoyer_donnees_fichier_serveur(int port, char * file_name) {
    int sockudp = connexion_udp(port);
    if (sockudp < 0) { perror("sock "); return 1; }
    
    //On récupère l'adresse du client auquel on veut envoyer les données.
    struct sockaddr_in6 adrclient;
    socklen_t len = sizeof(adrclient);
    char buffer[BUF_SIZE_UDP + 1];
    memset(buffer, '\0', BUF_SIZE_UDP + 1);

    int r = 0;
    int i = 0;

    //On bloque jusqu'à ce que le descripteur de lecture est disponible.
    while(i < 10000) {
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(sockudp, &rset);

        select(sockudp+1, &rset, NULL, 0, NULL);

        if (FD_ISSET(sockudp, &rset)) {
            r = recvfrom(sockudp, buffer, BUF_SIZE_UDP + 1, 0, (struct sockaddr *) &adrclient, &len);
            if (r < 0) { perror("recv "); return -1; }
            else break;
        }
        else {
            i += 1;
        }
    }

    if (r == 0) return -1;
    else return envoyer_donnees_fichier(sockudp, adrclient, 6, port, file_name, 0);
}