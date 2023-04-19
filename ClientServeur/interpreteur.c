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

#include "interpreteur.h"
#include "client.h"
#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"

#define BUF_SIZE 512
#define ID_LIMIT 10
#define MSG_LIMIT 255
#define PSEUDO_LIMIT 10
#define ENTETE_LEN 7

int main(int argc, char **argv)
{
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

                break;

            case 4: // s'abonner à un fil

                break;

            case 5: // poster un fichier
                poster_fichier_client(userid);
                break;

            case 6: // telecharger un fichier

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
    printf("msglen : %d\n", msglen);
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

int poster_fichier_client(int *userid) {
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
    if (sock == -1) { free(marray); free(str_input); return -1; }

    int size_exchanged = send(sock, marray, 12, 0);
    if (size_exchanged != 12)
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

    int r = envoyer_donnees_fichier(userid, str_input, rep.nb);
    if (r == -1) goto error;
    else printf("Fichier bien envoyé.\n");

    free(str_input);
    free(marray);
    return 0;

    error:
    printf("Erreur communication avec le serveur.\n");
    free(marray);
    free(str_input);
    close(sock);
    return -1;
}

int envoyer_donnees_fichier(int *userid, char * file_path, int port) {
    struct sockaddr_in6 adrclient;
    memset(&adrclient, 0, sizeof(adrclient));
    
    int sock = connexion_udp_6(&adrclient, port);
    if (sock < 0) { perror("sock "); return 1; }
    socklen_t len = sizeof(adrclient);

    int fd = open(file_path, O_RDONLY, 0640);
    if (fd == -1) {
        perror("open");
        close(sock);
        return -1;
    }
    char buffer[BUF_SIZE + 1];
    int numero_paq = 0;

    buffer[0] = numero_paq;

    int r = read(fd, buffer, BUF_SIZE + 1);
    if (r < 0) { 
        perror("read "); 
        close(sock);
        return -1; 
    }
    else if (r == 0) {
        printf("Le fichier que vous voulez envoyer est vide.\n");
        close(sock);
        return -1;
    }

    while (r > 0) {
        r = sendto(sock, buffer, BUF_SIZE + 1, 0, (struct sockaddr *)&adrclient, len);
        if (r < 0){ 
            perror("sendto ");
            close(sock);
            return -1;
        }

        memset(buffer, 0, BUF_SIZE + 1);
        r = read(fd, buffer, BUF_SIZE + 1);
        if (r < 0){ 
            perror("read ");
            close(sock);
            return -1; 
        }
    }

    close(sock);
    close(fd);
    
    return 0;
}