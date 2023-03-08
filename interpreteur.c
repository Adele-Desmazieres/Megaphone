#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define MSG_LIMIT 255


/*
    Crée un billet et l'envoie au serveur
*/
int poster_billet() {
    char str_input[MSG_LIMIT];
    printf("Entrez votre message > \n");
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
        // compris entre 0 et le nombre de valeurs possibles
        
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