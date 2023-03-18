#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "client.h"
#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"


#define MSG_LIMIT 255



int main() {
    
    
    
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
    close(sock);
    return 0;

    error:
    perror("Erreur connexion au serveur.\n"); 
    close(sock);
    return EXIT_FAILURE;

}




/*
    Crée un billet et l'envoie au serveur
*/
int poster_billet() {
    char str_input[MSG_LIMIT];
    printf("Entrez votre message > ");
    fgets(str_input, MSG_LIMIT, stdin);
    
    // TODO transformer la string en struct puis en tableau de données
    // puis l'envoyer au serveur
    
    return 0;
}

/*
    Interpreteur du côté utilisateur
*/
int interpreteur_utilisateur() {
    printf("Début de session.\n");
    
    char str_input[MSG_LIMIT];
    int session_continue = 1;
    
    while (session_continue) {
        
        printf("Que voulez-vous faire ?\n");
        fgets(str_input, MSG_LIMIT, stdin);
        
        // TODO vérifier que le charactère d'input est un entier
        // compris entre 1 et le nombre de valeurs possibles
        
        int nbr_input = atoi(str_input);
        
        switch (nbr_input) {
            
            case 1: // mettre fin à la session
                session_continue = 0;
                break;
            
            case 2: // poster un billet
                poster_billet();
                break;
            
            case 3: // demander la liste des n derniers billets
                
                break;
            
            case 4: // s'abonner à un fil
                
                break;
            
            case 5: // poster un fichier
                
                break;
            
            case 6: // telecharger un fichier
                
                break;
            
            default:
                break;
        }
    }
    
    // TODO gérer la fin de la session
    // mettre fin au programme utilisateur ? 
    // ou attendre qu'un nouveau identifiant soit entré ?
    
    return 0;
}