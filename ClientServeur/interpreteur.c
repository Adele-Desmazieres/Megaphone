#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

#include "client.h"
#include "interpreteur.h"
#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"

#define ID_LIMIT 10
#define MSG_LIMIT 255


int main(int argc, char **argv) {
    printf("Initialisation du programme client.\n");    
    int *userid = malloc(sizeof(int));    
    int ret = 1;
    
    while (ret != 0) {
        ret = inscription_ou_connexion(userid);        
    }
    
    
    
    return 0;
}

/*
    Page d'accueil lors du lancement du client. 
    L'utilisateur peut choisir de s'inscrire ou de se connecter. 
*/
int inscription_ou_connexion(int *userid) {
    printf("Page d'accueil.\n");
    char str_input[MSG_LIMIT];
    
    printf("Que voulez-vous faire ? Entrez 1 pour vous inscrire, 2 pour vous connecter, et 3 pour arrêter le programme.\n");
    fgets(str_input, MSG_LIMIT, stdin);
    
    while (!isdigit(str_input[0]) || atoi(str_input) > 3 || atoi(str_input) < 0) {
        printf("Veuillez entrer le nombre 1, 2 ou 3.\n");
    }
    
    int nbr_input = atoi(str_input);
    int ret = 1;
    switch (nbr_input) {
            
        case 1: // inscription
            ret = inscription(userid);
            break;
            
        case 2: // connexion
            ret = connexion_6(userid);
            break;
            
        case 3: // arret du programme
            // TODO
            break;
        
        default: 
            printf("Normalement on ne tombe jamais ici car des vérif préalables l'en empêche.\n");
            break;
        
    }
    
    return ret;        
}


/*
    Interpreteur du côté utilisateur
*/
int interpreteur_utilisateur(int *userid) {
    printf("Début de session.\n");
    
    char str_input[MSG_LIMIT];
    int session_continue = 1;

    while (session_continue) {
        
        printf("Que voulez-vous faire ? Entrez un nombre entre 1 et 7 inclus. Entrez 7 pour afficher les différentes commandes.\n");
        fgets(str_input, MSG_LIMIT, stdin);

        if (!isdigit(str_input[0]) || atoi(str_input) > 7 || atoi(str_input) < 0) {
            printf("Veuillez entrer un nombre entre 1 et 7 inclus.\n");
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
                    printf("1 : déconnexion\n");
                    printf("2 : poster un message\n");
                    printf("3 : afficher des messages\n");
                    printf("4 : s'abonner à un fil\n");
                    printf("5 : poster un fichier\n");
                    printf("6 : télécharger un fichier\n");
                    printf("7 : affichage des commandes\n");
                    break;
                    
                default:
                    printf("Ce nombre n'est pas dans l'intervalle de 1 à 7. Veuillez entrer un nombre entre 1 et 7 inclus.\n");
                    break;
            }
        }
    }

    return 0;
}

/*
    Inscrit l'utilisateur : 
    - en cas de réussite, initialise userid et renvoie 0
    - sinon renvoie 1
*/ 
int inscription(int *userid) {
    char str_input[10];
    printf("Entrez votre pseudo > ");
    fgets(str_input, 10, stdin); // 10 = taille max du pseudo, imposé par le sujet
    int n = strlen(str_input);
    // quitte l'inscription si rien n'est entré
    if (n == 0) {
        return 1;
    }
    // redemande un pseudo s'il est trop long
    while (n > 10) { // TODO : vérifier que le pseudo contient que des caractères alphanum ?
        printf("Votre pseudo peut contenir au plus 10 caractères.\n");
        printf("Entrez votre pseudo > ");
        fgets(str_input, 10, stdin);
        int n = strlen(str_input);
        if (n == 0) {
            return 1;
        }
    }
    
    // envoie le message contenant le pseudo en suivant le protocole
    msg_client *mstruct = msg_client_constr(1, 0, -1, -1, n, str_input, 1);
    u_int16_t *marray = msg_client_to_send(*mstruct);
    int sock = connexion_6();
    int size_exchanged = send(sock, marray, 6, 0); // TODO est-ce bien ca qu'il faut envoyer ? taille 6 ?
    if (size_exchanged != 6) goto error;
    
    // recoit l'identifiant en réponse
    u_int16_t buff[3];
    size_exchanged = recv(sock, buff, 3, 0);
    if (size_exchanged != 3) goto error;
    
    // interprète la réponse
    msg_serveur *rep = tcp_to_msgserveur(buff);
    if (rep->codereq != 1) { // échec du côté serveur
        printf("Echec de l'inscription côté serveur.");
        return 1;
    }
    
    // réussite
    *userid = rep->id;
    printf("Prennez en note votre identifiant : %d\n", *userid);
    close(sock);
    return 0;

    error:
    perror("Erreur connexion au serveur.\n"); 
    close(sock);
    return EXIT_FAILURE;
}

/*
    Connexion de l'utilisateur avec son identifiant. 
    Ne fait aucune vérif quant à l'existence de l'identifiant dans les données du serveur. 
    Renvoie 0 si réussite, et 1 sinon. 
*/
int connexion(int *userid) {
    char str_input[ID_LIMIT];
    printf("Entrez votre identifiant donné à l'inscription > ");
    fgets(str_input, 10, stdin); // 10 = taille max du pseudo, imposé par le sujet
    int n = strlen(str_input);
    // quitte la connexion si rien n'est entré
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
    printf("Identifiant %d enregistré.\n", *userid);
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