#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

#include "interpreteur.h"
#include "client.h"
#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"

#define ID_LIMIT 10
#define MSG_LIMIT 255
#define PSEUDO_LIMIT 10


int main(int argc, char **argv) {
    printf("Initialisation du programme client.\n");
    int *userid = malloc(sizeof(int));
    *userid = -1; // userid négatif = pas set
    int ret = 1;
    
    while (*userid < 0) {
        while (ret != 0) {
            ret = inscription_ou_debut_session(userid);        
        }
    }
    
    printf("Début de session utilisateur avec l'identifiant %d.\n", *userid);
    
    ret = interpreteur_utilisateur(userid);    
    
    printf("Fin de session utilisateur.\n");
    return 0;
}


/*
    Arrete le progamme client, free les pointeurs. 
*/
int arret(int *userid) {
    printf("Arrêt du programme client.\n");
    free(userid);
    exit(0);
}


/*
    Page d'accueil lors du lancement du client. 
    L'utilisateur peut choisir de s'inscrire ou de démarrer sa session. 
*/
int inscription_ou_debut_session(int *userid) {
    printf("\nPage d'accueil.\n");
    char str_input[MSG_LIMIT];
    
    printf("1 : inscription\n2 : démarrer une session\n3 : arrêt du programme\nQue voulez-vous faire ? > ");
    fgets(str_input, MSG_LIMIT, stdin);
    
    while (!isdigit(str_input[0]) || atoi(str_input) > 3 || atoi(str_input) < 1) {
        printf("Réponse non reconnue.\n1 : inscription\n2 : démarrer une session\n3 : arrêt du programme\nQue voulez-vous faire ? > ");
        fgets(str_input, MSG_LIMIT, stdin);
    }
    
    int nbr_input = atoi(str_input);
    int ret = 1;
    switch (nbr_input) {
            
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
    - sinon renvoie 1
*/ 
int inscription(int *userid) {
    char str_input[MSG_LIMIT];
    int n = PSEUDO_LIMIT + 1;
    
    while (n > PSEUDO_LIMIT) {
    
        printf("Entrez votre pseudo > ");
        fgets(str_input, MSG_LIMIT, stdin); // TODO : corriger le bug qui fait que si un texte plus long que MSG_LIMIT est entré, alors les MSG_LIMIT premiers caractères seront écrit dans ce pseudo (qui sera refusé car trop long) mais la fin du texte sera écrit dans la demande de pseudo suivante (qui peut être accepté si ca rentre dans la limite de longueur du pseudo)
        
        str_input[MSG_LIMIT-1] = '\0'; // remplace le dernier caractère par \0 pour le cas où le pseudo entré est trop long
    
        n = strlen(str_input);    
        // on enlève le \n à la fin de la chaîne récupérée, s'il est le dernier caractère
        if (str_input[n - 1] == '\n') {
            str_input[n - 1] = '\0';
            n = n-1;
        }
        printf("longueur : %d\n", n);
        
        // quitte l'inscription si rien n'est entré
        if (n <= 0) {
            return 1;
        } else if (n > PSEUDO_LIMIT) { // TODO : check pseudo contient que des caractères alphanum ?
            printf("Pseudo trop long. Il doit faire au plus %d caractères.\n", PSEUDO_LIMIT);
        }
    }
    
    // envoie le message contenant le pseudo en suivant le protocole
    msg_client *mstruct = msg_client_constr(1, 0, -1, -1, n, str_input, 1);
    
    //printf("PseudoClient : %s\n", mstruct->data);
    u_int16_t *marray = msg_client_to_send(*mstruct);

    int sock = connexion_6();
    if (sock == -1) return EXIT_FAILURE;
    int size_exchanged = send(sock, marray, 12, 0); 
    if (size_exchanged != 12) goto error;
    
    // recoit l'identifiant en réponse
    u_int16_t buff[3];
    size_exchanged = recv(sock, buff, 6, 0);
    if (size_exchanged != 6) goto error;
    
    // interprète la réponse
    msg_serveur *rep = tcp_to_msgserveur(buff);
    if (rep->codereq != 1) { // échec du côté serveur
        printf("Echec de l'inscription côté serveur.\n");
        return 1;
    }
    
    // réussite
    *userid = rep->id;
    printf("Prennez en note votre identifiant : %d\n", *userid);
    close(sock);
    return 0;

    error:
    printf("Erreur communication avec le serveur.\n");
    close(sock);
    return EXIT_FAILURE;
}


/*
    Démarrage d'une session utilisateur avec son identifiant. 
    Ne fait aucune vérif quant à l'existence de l'identifiant dans les données du serveur. 
    Renvoie 0 si réussite, et 1 sinon. 
*/
int debut_session(int *userid) {
    char str_input[ID_LIMIT];
    printf("Entrez votre identifiant donné à l'inscription > ");
    fgets(str_input, 10, stdin); // 10 = taille max du pseudo, imposé par le sujet
    int n = strlen(str_input);
    // quitte la fonction si rien n'est entré
    if (n == 0) {
        return 1;
    }
    // redemande un identifiant s'il est trop long
    while (!isdigit(str_input[0])) { 
        printf("Votre identifiant doit être un nombre entier > ");
        fgets(str_input, ID_LIMIT, stdin);
        int n = strlen(str_input);
        if (n == 0) {
            return 1;
        }
    }
    
    // réussite de la connexion
    *userid = atoi(str_input);
    return 0;
}


/*
    Interpreteur du côté utilisateur
*/
int interpreteur_utilisateur(int *userid) {
    printf("\nPage principale.\n");
    
    char str_input[MSG_LIMIT];
    int session_continue = 1;

    while (session_continue) {
        
        lister_commandes_en_session();
        printf(" Entrez 7 pour afficher les différentes commandes.\nQue voulez-vous faire ? > ");
        fgets(str_input, MSG_LIMIT, stdin);

        if (!isdigit(str_input[0]) || atoi(str_input) > 7 || atoi(str_input) < 0) {
            printf("Réponse non reconnue.\n");
        }

        else {
            int nbr_input = atoi(str_input);
            
            switch (nbr_input) {
            
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

    return 0;
}


/*
    Crée un billet et l'envoie au serveur
*/
int poster_billet_client(int *userid) {
    char str_input[MSG_LIMIT];
    printf("Entrez votre message > ");
    fgets(str_input, MSG_LIMIT, stdin);
    
    // TODO transformer la string en struct puis en tableau de données
    // puis l'envoyer au serveur
    
    return 0;
}


int lister_commandes_en_session() {
    printf("1 : fin de session\n");
    printf("2 : poster un message\n");
    printf("3 : afficher des messages\n");
    printf("4 : s'abonner à un fil\n");
    printf("5 : poster un fichier\n");
    printf("6 : télécharger un fichier\n");
    printf("7 : affichage des commandes\n");
    return 0;
}