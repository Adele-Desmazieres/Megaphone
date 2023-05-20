#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../UDP/msg_multicast.h"
#include <poll.h>
#include <pthread.h>
#include <sys/select.h>

#include "interpreteur.h"

#define PORT 2121
#define BUF_SIZE 512
#define ID_LIMIT 10
#define MSG_LIMIT 255
#define PSEUDO_LIMIT 10
#define ENTETE_LEN 7

pthread_mutex_t verrou_pollfd = PTHREAD_MUTEX_INITIALIZER;

struct pollfd * notrepoll;
int * tailledepoll;

int main(int argc, char **argv)
{
    notrepoll = malloc(sizeof(struct pollfd));
    tailledepoll = malloc(sizeof(int));
    int pollinit = 0;
    memcpy(tailledepoll, &pollinit, sizeof(int));

    //Création du thread et lancement de sa routine
    pthread_t thread;
    if(pthread_create(&thread, NULL, thread_notifs, NULL) == -1){
        perror("Echec connexion\n");
    }

    printf("Initialisation du programme client.\n");
    int *userid = malloc(sizeof(int));
    if (userid == NULL) return 1;

    *userid = -1; // userid négatif = pas set
    int ret = 1;

    while (*userid < 0)
    {
        while (ret != 0)
        {
            ret = inscription_ou_debut_session(userid);
        }
    }

    printf("Début de session utilisateur avec l'identifiant %d.\n", *userid);

    ret = interpreteur_utilisateur(userid);

    printf("Fin de session utilisateur.\n");
    return ret;
}

/*
    Arrete le progamme client, free les pointeurs.
*/
int arret(int *userid)
{
    printf("Arrêt du programme client.\n");
    free(userid);
    exit(0);
}

/*
    Page d'accueil lors du lancement du client.
    L'utilisateur peut choisir de s'inscrire ou de démarrer sa session.
*/
int inscription_ou_debut_session(int *userid)
{
    printf("\nPage d'accueil.\n");
    char str_input[MSG_LIMIT];

    printf("1 : inscription\n2 : démarrer une session\n3 : arrêt du programme\nQue voulez-vous faire ? > ");
    fgets(str_input, MSG_LIMIT, stdin);

    while (!isdigit(str_input[0]) || atoi(str_input) > 3 || atoi(str_input) < 1)
    {
        printf("Réponse non reconnue.\n1 : inscription\n2 : démarrer une session\n3 : arrêt du programme\nQue voulez-vous faire ? > ");
        fgets(str_input, MSG_LIMIT, stdin);
    }

    int nbr_input = atoi(str_input);
    int ret = 1;
    switch (nbr_input)
    {

    case 1: // inscription
        ret = inscription(userid);
        break;

    case 2: // démarrer une session
        ret = debut_session(userid);
        break;

    case 3: // arret du programme
        arret(userid);
        break;

    default:
        printf("Numéro d'action non-reconnue.\n");
        break;
    }

    return ret;
}

/*
    Inscrit l'utilisateur :
    - en cas de réussite, initialise userid et renvoie 0
    - sinon renvoie -1
*/
int inscription(int *userid)
{
    char str_input[MSG_LIMIT];
    int n = PSEUDO_LIMIT + 1;

    while (n > PSEUDO_LIMIT)
    {

        printf("Entrez votre pseudo > ");
        fgets(str_input, MSG_LIMIT, stdin); // TODO : corriger le bug qui fait que si un texte plus long que MSG_LIMIT est entré, alors les MSG_LIMIT premiers caractères seront écrit dans ce pseudo (qui sera refusé car trop long) mais la fin du texte sera écrit dans la demande de pseudo suivante (qui peut être accepté si ca rentre dans la limite de longueur du pseudo)

        str_input[MSG_LIMIT - 1] = '\0'; // remplace le dernier caractère par \0 pour le cas où le pseudo entré est trop long

        n = strlen(str_input);
        // on enlève le \n à la fin de la chaîne récupérée, s'il est le dernier caractère
        if (str_input[n - 1] == '\n')
        {
            str_input[n - 1] = '\0';
            n = n - 1;
        }

        // quitte l'inscription si rien n'est entré
        if (n <= 0)
        {
            return -1;
        }
        else if (n > PSEUDO_LIMIT)
        { // TODO : check pseudo contient que des caractères alphanum ?
            printf("Pseudo trop long. Il doit faire au plus %d caractères.\n", PSEUDO_LIMIT);
        }
    }

    // crée le message contenant le pseudo en suivant le protocole²
    msg_client mstruct = { 1, 0, -1, -1, n, str_input, 1 };
    u_int16_t *marray = msg_client_to_send(mstruct);

    // envoie le message
    int sock = connexion_6();
    if (sock == -1) {
        free(marray);
        return -1;
    }

    int size_exchanged = send(sock, marray, 12, 0);
    if (size_exchanged != 12)
        goto error;

    // recoit l'identifiant en réponse
    u_int16_t buff[3];
    size_exchanged = recv(sock, buff, 6, 0);
    if (size_exchanged != 6)
        goto error;

    // interprète la réponse
    msg_serveur rep = tcp_to_msgserveur(buff);
    if (rep.codereq != 1)
    { // échec du côté serveur
        printf("Echec de l'inscription côté serveur.\n");
        free(marray);
        return -1;
    }

    // réussite
    *userid = rep.id;
    printf("Prennez en note votre identifiant : %d\n", *userid);
    free(marray);
    close(sock);
    return 0;

error:
    printf("Erreur communication avec le serveur.\n");
    free(marray);
    close(sock);
    return -1;
}

/*
    Démarrage d'une session utilisateur avec son identifiant.
    Ne fait aucune vérif quant à l'existence de l'identifiant dans les données du serveur.
    Renvoie 0 si réussite, et -1 sinon.
*/
int debut_session(int *userid)
{
    char str_input[ID_LIMIT];
    printf("Entrez votre identifiant donné à l'inscription > ");
    fgets(str_input, 10, stdin); // 10 = taille max du pseudo, imposé par le sujet
    int n = strlen(str_input);
    // quitte la fonction si rien n'est entré
    if (n <= 1)
    {
        return -1;
    }
    // redemande un identifiant s'il est trop long
    while (!isdigit(str_input[0]))
    {
        printf("Votre identifiant doit être un nombre entier > ");
        fgets(str_input, ID_LIMIT, stdin);
        int n = strlen(str_input);
        if (n <= 1)
        {
            return -1;
        }
    }

    // réussite de la connexion
    *userid = atoi(str_input);
    return 0;
}

/*
    Interpreteur du côté utilisateur
*/
int interpreteur_utilisateur(int *userid)
{
    printf("\nPage principale.\n");

    char str_input[MSG_LIMIT];
    int session_continue = 1;

    while (session_continue)
    {
        printf("\n");
        lister_commandes_en_session();
        printf("Entrez 7 pour afficher les différentes commandes.\nQue voulez-vous faire ? > ");
        fgets(str_input, MSG_LIMIT, stdin);

        if (!isdigit(str_input[0]) || atoi(str_input) > 7 || atoi(str_input) < 0)
        {
            printf("Réponse non reconnue.\n");
        }

        else
        {
            int nbr_input = atoi(str_input);
            switch (nbr_input)
            {

            case 1: // mettre fin à la session
                session_continue = 0;
                break;

            case 2: // poster un billet
                poster_billet_client(userid);
                break;

            case 3: // demander la liste des n derniers billets
                get_n_billets(*userid);
                break;

            case 4: // s'abonner à un fil
                abonnement_fil(*userid);
                break;

            case 5: // poster un fichier
                envoyer_donnees_fichier_client(userid);
                break;

            case 6: // telecharger un fichier
                recevoir_donnees_fichier_client(userid);
                break;

            case 7: // lister des commandes
                lister_commandes_en_session();
                break;

            default:
                printf("Réponse non reconnue.\n");
                break;
            }
        }
    }

    pthread_mutex_lock(&verrou_pollfd);
    free(notrepoll);
    free(tailledepoll);
    pthread_mutex_unlock(&verrou_pollfd);

    free(userid);

    return 0;
}

/*
    Affiche les commandes accessibles lorsqu'un utilisateur est en cours de session.
*/
int lister_commandes_en_session()
{
    printf("1 : fin de session\n");
    printf("2 : poster un message\n");
    printf("3 : afficher des messages\n");
    printf("4 : s'abonner à un fil\n");
    printf("5 : poster un fichier\n");
    printf("6 : télécharger un fichier\n");
    printf("7 : affichage des commandes\n");
    return 0;
}

/*
    Vérifie qu'un string ne contient que des nombres, et au moins un charactère.
*/
int string_is_number(char *s)
{
    if (s == NULL || s[0] == '\0')
        return 0;
    for (int i = 0; i < strlen(s); i++)
    {
        if (!isdigit(s[i]))
        {
            return 0;
        }
    }
    return 1;
}

// https://stackoverflow.com/questions/8164000/how-to-dynamically-allocate-memory-space-for-a-string-and-get-that-string-from-u
// TODO : à remplacer par readline
char *getln()
{
    char *line = NULL;
    char *tmp = NULL;
    size_t size = 0;
    size_t index = 0;
    int c = EOF;

    while (c)
    {
        c = getc(stdin);

        // Check if we need to stop, and replace the ending-char by the \0
        if (c == EOF || c == '\n')
        {
            c = 0;
        }

        // Check if we need to expand
        if (size <= index)
        {
            size += MSG_LIMIT;
            tmp = realloc(line, size);
            if (!tmp)
            {
                free(line);
                line = NULL;
                break;
            }
            line = tmp;
        }

        // Actually store the character
        line[index++] = c;
    }
    return line;
}

/*
    Crée un billet et l'envoie au serveur
*/
int poster_billet_client(int *userid)
{
    char *str_input;
    char *num_input;
    int n;

    //On prend le numéro du fil dans lequel l'utilisateur veux poster.
    printf("Entrez le numéro du fil sur lequel envoyer le message, ou 0 pour un nouveau fil > ");
    num_input = getln();
    while (!string_is_number(num_input) || strlen(num_input) <= 0) {
        printf("Non, veuillez entrer un numéro de fil > ");
        free(num_input);
        num_input = getln();
    }
    int numfil = atoi(num_input);
    free(num_input);

    // On prend le message que l'utilisatur veut écrire dans le billet.
    printf("Entrez votre message > ");
    str_input = getln();
    n = strlen(str_input);
    
    // crée le message
    msg_client mstruct = {2, *userid, numfil, 0, n, str_input, 0};
    u_int16_t *marray = msg_client_to_send(mstruct);

    // l'envoie
    int sock = connexion_6();
    if (sock == -1) {
        free(marray);
        return -1;
    }
    int msglen = n + ENTETE_LEN;
    int size_exchanged = send(sock, marray, msglen, 0);
    if (size_exchanged != msglen)
        goto error;

    // recoit la réponse
    u_int16_t buff[3];
    size_exchanged = recv(sock, buff, 6, 0);
    if (size_exchanged != 6)
        goto error;

    // interprète la réponse
    msg_serveur rep = tcp_to_msgserveur(buff);
    if (rep.codereq != 2)
    { // échec du côté serveur
        free(marray);
        free(mstruct.data);
        printf("Echec du traitement côté serveur.\n");
        return -1;
    }

    // réussite
    free(marray);
    free(mstruct.data);
    printf("1 message envoyé sur le fil %d.\n", rep.numfil);
    close(sock);
    return 0;

    error:
    printf("Erreur communication avec le serveur.\n");
    free(marray);
    free(mstruct.data);
    close(sock);
    return -1;
}

/*
    Envoie un billet avec le nom d'un fichier, ainsi que le contenu du fichier au serveur.
*/
int envoyer_donnees_fichier_client(int *userid) {
    char *str_input;
    char *num_input;
    int n;

    //On prend le numéro du fil dans lequel l'utilisateur veux poster le fichier.
    printf("Entrez le numéro du fil sur lequel vous voulez poster votre fichier, ou 0 pour un nouveau fil > ");
    num_input = getln();
    while (!string_is_number(num_input) || strlen(num_input) <= 0) {
        printf("Veuillez entrer un numéro correct > ");
        free(num_input);
        num_input = getln();
    }
    int numfil = atoi(num_input);
    free(num_input);

    //On prend le nom/chemin du fichier.
    printf("Entrez le nom du fichier > ");
    str_input = getln();
    n = strlen(str_input);

    //On vérifie que ce fichier existe bien.
    struct stat * buf = malloc(sizeof(struct stat));
    if (buf == NULL) { perror("malloc "); return -1; }
    if (stat(str_input, buf) != 0) {
        printf("Le fichier que vous avez entré n'est pas trouvable.\n");
        free(buf);
        free(str_input);
        return -1;
    }

    free(buf);

    // On créé le message avec lendata le nombre de caractère du fichier et data le nom du fichier.
    msg_client mstruct = {5, *userid, numfil, 0, strlen(str_input), str_input, 0 };
    u_int16_t *marray = msg_client_to_send(mstruct);

    // l'envoie
    int sock = connexion_6();
    if (sock == -1) {
        free(marray);
        return -1;
    }
    int msglen = n + ENTETE_LEN;
    int size_exchanged = send(sock, marray, msglen, 0);
    if (size_exchanged != msglen)
        goto error;

    // recoit la réponse
    u_int16_t buff[3];
    size_exchanged = recv(sock, buff, 6, 0);
    if (size_exchanged != 6)
        goto error;

    // interprète la réponse
    msg_serveur rep = tcp_to_msgserveur(buff);
    if (rep.codereq != 5) { // échec du côté serveur
        printf("Echec du traitement côté serveur.\n");
        free(marray);
        free(str_input);
        return -1;
    }

    // réussite
    printf("Numero de port serveur reçu : %d.\n", rep.nb);
    close(sock);

    struct sockaddr_in6 adrudp;
    memset(&adrudp, 0, sizeof(adrudp));

    int sockudp = connexion_udp_6(&adrudp, rep.nb);
    if (sockudp < 0) { perror("sock "); return 1; }

    int r = envoyer_donnees_fichier(sockudp, adrudp, 5, rep.nb, str_input);

    close(sockudp);
    free(str_input);
    free(marray);

    if (r == -1) { printf("Fichier mal envoyé.\n"); return -1; }
    else printf("Fichier bien envoyé.\n");

    return 0;

    error:
    printf("Erreur communication avec le serveur.\n");
    free(marray);
    free(str_input);
    close(sock);
    return -1;
}

/*
    Recoit le fichier d'un billet donné et écrit son contenu dans un autre passé en parametres.
*/
int recevoir_donnees_fichier_client(int *userid) {
    char *str_input;
    char *num_input;
    int n;

    //On prend le numéro du fil dans lequel l'utilisateur veux poster le fichier.
    printf("Entrez le numéro du fil sur lequel vous voulez votre fichier > ");
    num_input = getln();
    while (!string_is_number(num_input) || strlen(num_input) <= 0) {
        printf("Veuillez entrer un numéro correct > ");
        free(num_input);
        num_input = getln();
    }
    int numfil = atoi(num_input);
    free(num_input);

    //On prend le nom/chemin du fichier que l'on veut recuperer
    printf("Entrez le nom du fichier que vous voulez recuperer > ");
    str_input = getln();
    n = strlen(str_input);

    //On prend le nom/chemin du fichier ou l'on enverra les donnees
    printf("Entrez le nom du fichier ou vous voulez ecrire les donnees > ");
    char * recep_file = getln();

    // On créé le message avec lendata le nombre de caractère du fichier et data le nom du fichier.
    msg_client mstruct = {6, *userid, numfil, PORT, strlen(str_input), str_input, 0};
    u_int16_t *marray = msg_client_to_send(mstruct);

    // l'envoie
    int sock = connexion_6();
    if (sock == -1) { free(marray); return -1; }
    
    int msglen = n + ENTETE_LEN;
    int size_exchanged = send(sock, marray, msglen, 0);
    if (size_exchanged != msglen)
        goto error;

    // recoit la réponse
    u_int16_t buff[3];
    size_exchanged = recv(sock, buff, 6, 0);
    if (size_exchanged != 6)
        goto error;

    // interprète la réponse
    msg_serveur rep = tcp_to_msgserveur(buff);
    if (rep.codereq != 6) { // échec du côté serveur
        printf("Le fichier que vous vouliez prendre n'existe pas.\n");
        free(marray);
        free(str_input);
        return -1;
    }

    // réussite
    close(sock);

    struct sockaddr_in6 servadr;
    memset(&servadr, 0, sizeof(servadr));

    int sockudp = connexion_udp_6(&servadr, PORT);
    if (sockudp < 0) { perror("sock "); return 1; }

    //On envoie au serveur un paquet pour qu'il puisse identifier l'adresse client.
    int r = envoyer_serveur_udp_adr(servadr, sock);
    if (r == -1) { 
        close(sockudp); 
        free(recep_file);
        free(str_input);
        free(marray);
        printf("Erreur communication serveur "); 
        return -1; 
    }

    r = recevoir_donnees_fichier(sockudp, recep_file);

    if (r == -1) { perror("Fichier mal reçu");}
    else printf("Fichier bien reçu.\n");

    close(sockudp);
    free(recep_file);
    free(str_input);
    free(marray);

    if (r == -1) { printf("Fichier mal envoyé.\n"); return -1; }
    else printf("Fichier bien envoyé.\n");

    return 0;

    error:
    printf("Erreur communication avec le serveur.\n");
    free(marray);
    free(str_input);
    close(sock);
    return -1;
}

/*
    Envoie un paquet au fichier pour que celui-ci puisse identifier l'adresse du client.
*/
int envoyer_serveur_udp_adr(struct sockaddr_in6 servadr, int sock) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    sprintf(buffer, "Message pour serv.");

    socklen_t len = sizeof(servadr);

    int i = 0;
    int r = 0;

    //On bloque jusqu'à ce que le descripteur d'écriture est disponible.
    while(i < 10000){
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(sock, &wset); //pour surveillance en écriture de sock
        select(sock+1, NULL, &wset, 0, NULL);

        if (FD_ISSET(sock, &wset)) {
            //On envoie un message pour que le serveur puisse récuperer l'adresse du client
            r = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *) &servadr, len);
            if (r < 0) { perror("Echec sendto "); return -1; }
            i ++;
        }
    }

    return 0;
}

int abonnement_fil(int userid){

    int sockfd = connexion_6();
    if(sockfd < 0){
        perror("Erreur de connexion @ abonnement_fil\n");
        return -1;
    }

    //TODO récupérer les informations par l'interpréteur : numéro de fil

    char *num_input;

    printf("Entrez le numéro du fil auquel vous souhaitez vous abonner > ");
    num_input = getln();
    while (!string_is_number(num_input) || strlen(num_input) <= 0) {
        printf("Veuillez entrer un numéro correct > ");
        free(num_input);
        num_input = getln();
    }
    int numfil = atoi(num_input);
    free(num_input);

    msg_client requete = { .codereq = 4, .id = userid, .numfil = numfil, .datalen = 0, .data = "", .is_inscript = 0};
    u_int16_t * buf = msg_client_to_send(requete);

    if (send(sockfd, buf, 6, 0) < 0){
        perror("Erreur envoi requête @ abonnemnt_fil \n"); return -1;
    }

    free(buf);

    msg_demande_abo * infos_ip = tcp_to_msg_abo(sockfd);
    
    if(infos_ip->codereq == 31){
        printf("Erreur côté serveur\n"); return -1;
    }

    int sock_multicast;
    pthread_mutex_lock(&verrou_pollfd);
    if(*tailledepoll == 0){
        sock_multicast = socket(AF_INET6, SOCK_DGRAM, 0);
        if (sock_multicast < 0){
            pthread_mutex_unlock(&verrou_pollfd);
            perror("Erreur socket @ abonnement_fil\n"); return -1;
        }

        int reuse = 1;
        if (setsockopt(sock_multicast, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
            perror("erreur option reuse addr @ abonnement_fil \n");
        }

        struct sockaddr_in6 grsock = {0};
        grsock.sin6_family = AF_INET6;
        grsock.sin6_addr = in6addr_any;
        grsock.sin6_port = htons(PORT_MULTICAST);

        if(bind(sock_multicast, (struct sockaddr * ) &grsock, sizeof(grsock)) < 0){
            perror("Erreur bind @ abonnement_fil \n");
        }

    } else {
        sock_multicast = notrepoll[0].fd;
    }

    struct ipv6_mreq group = {0};
    //printf("IPv6 reçue? %s \n", infos_ip->ip);
    inet_pton(AF_INET6, infos_ip->ip, &group.ipv6mr_multiaddr);
    group.ipv6mr_interface = 0;

    if (setsockopt(sock_multicast, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group)) < 0){
        perror("erreur option join group @ abonnement_fil \n");
    }

    if(*tailledepoll == 0){
        struct pollfd poll_specifique = {0};
        poll_specifique.fd = sock_multicast;
        poll_specifique.events = POLLIN;

        notrepoll[0] = poll_specifique;

        *tailledepoll = 1;
    }
    pthread_mutex_unlock(&verrou_pollfd);


    printf("Abonné avec succès au fil %d\n", infos_ip->numfil);

    free(infos_ip->ip);
    free(infos_ip);

    return 0;
}

void * thread_notifs(void * args){

    while(1){

        pthread_mutex_lock(&verrou_pollfd);
        if(*tailledepoll == 0){ pthread_mutex_unlock(&verrou_pollfd); continue; }

        if( (poll(notrepoll, *tailledepoll, 5000)) == 0) { pthread_mutex_unlock(&verrou_pollfd); continue; }
            
            if ((notrepoll[0].revents & POLLIN) == 0) continue;
            if (notrepoll[0].revents & POLLIN){
                //Alors ce descripteur est prêt pour la lecture;

                u_int16_t buf[SIZE_MSG_NOTIF] = {0};
                if (read(notrepoll[0].fd, buf, SIZE_MSG_NOTIF) <= 0){
                    perror("Read sur descripteur de notif\n");
                }

                msg_notif * msg_a_afficher = udp_to_msg_notif(buf);

                printf("\nNotif fil %d : %s - %s \n", msg_a_afficher->numfil, msg_a_afficher->data, msg_a_afficher->pseudo);
                notrepoll[0].revents = 0;

                free(msg_a_afficher->pseudo);
                free(msg_a_afficher->data);
                free(msg_a_afficher);                    
            }
        pthread_mutex_unlock(&verrou_pollfd);
    }
}

int get_n_billets(int userid){

    int sockfd = connexion_6();
    if(sockfd < 0){
        perror("connexion_6 @ get_n_billets \n");
    }

    char *num_input;

    printf("Entrez le numéro du fil duquel vous souhaitez récupérer les derniers messages (0 pour tous) > ");
    num_input = getln();
    while (!string_is_number(num_input) || strlen(num_input) <= 0) {
        printf("Veuillez entrer un numéro correct > ");
        free(num_input);
        num_input = getln();
    }
    int numfil = atoi(num_input);
    free(num_input);

    printf("Entrez le nombre de derniers messages que vous souhaitez récupérer (0 pour tous) > ");
    num_input = getln();
    while (!string_is_number(num_input) || strlen(num_input) <= 0) {
        printf("Veuillez entrer un numéro correct > ");
        free(num_input);
        num_input = getln();
    }
    int nb = atoi(num_input);
    free(num_input);

    msg_client requete = {.codereq = 3, .numfil = numfil, .id = userid, .nb = nb, .datalen = 0, .data = "", .is_inscript = 0};
    u_int16_t * req_to_snd = msg_client_to_send(requete);

    if( send(sockfd, req_to_snd, 7, 0) < 0){
        perror("Erreur envoi requete @ get_n_billets\n");
    }

    free(req_to_snd);

    u_int16_t * prem_reponse = malloc(SIZE_MSG_SERVEUR);
    memset(prem_reponse, 0, SIZE_MSG_SERVEUR);
    if( recv(sockfd, prem_reponse, SIZE_MSG_SERVEUR, 0) < 0){
        perror("Erreur réception répone @ get_n_billets\n");
    }

    msg_serveur infos_reponse = tcp_to_msgserveur(prem_reponse);
    free(prem_reponse);

    if(infos_reponse.codereq == 31){
        printf("Erreur envoyée par le serveur, vérifiez vos informations\n");
        return -1;
    }

    if(numfil == 0){
        printf("Il y a actuellement %d fils actifs sur le serveur\n", infos_reponse.numfil);
    }

    for(int i = 0; i < infos_reponse.nb; i++){

        msg_billet_envoi * billet_recu = tcp_to_msgbillet(sockfd);
        printf("Fil %d initié par %s \n %s dit \' %s \' \n\n", billet_recu->numfil, billet_recu->origine, billet_recu->pseudo, billet_recu->data);
        free(billet_recu->data);
        free(billet_recu->pseudo);
        free(billet_recu->origine);
        free(billet_recu);

    }

    return 0;
}