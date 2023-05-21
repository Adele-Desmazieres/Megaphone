#ifndef BDD_SERVEUR
#define BDD_SERVEUR

//STRUCTURE BILLET
typedef struct billet billet;
typedef struct billet {
    char * texte;
    char * auteur;
    int is_new;
    billet * prec;
    billet * suiv;
} billet;

//STRUCTURE FIL
typedef struct fil fil;
typedef struct fil {

    int id;
    int nb_de_msg;
    billet * premier_msg;
    
    int is_multicast;
    int multicast_sockfd;
    struct sockaddr_in6 * sockopt;
    char * multicast_addr;

    fil * suiv;
    
} fil;

//LISTE DE FILS
typedef struct liste_fils{
    int nb_de_fils;
    fil * premier_fil;
} liste_fils;

//NOEUD POUR LA LISTE DES UTILISATEURS INSCRITS

typedef struct user_listnode user_listnode;
typedef struct user_listnode {

    char * pseudo;
    int id;

    user_listnode * suiv;

} user_listnode;

//LISTE DES UTILISATEURS INSCRITS

typedef struct user_list {

    int len;
    user_listnode * first;

} user_list;

//CONSTRUIS UN BILLET, THEORIQUEMENT INTERNE
billet * billet_constr(char * auteur, char * texte, int is_new_for_notif);

//CONSTRUIS UN FIL A PARTIR D'UN AUTEUR ET UN TEXTE
fil * fil_constr(char * auteur, char * texte);
//Récupère le fil par le numéro de fil passé en parametre, renvoie NULL si non trouvé.
fil * get_fil_id(liste_fils * l, int numfil);
//AJOUTE UN BILLET (AUTEUR, TEXTE) A UN FIL F
void ajouter_billet(fil * f, char * auteur, char * texte, int is_new_for_notif);
//RENVOIE LES N DERNIERS BILLET DU FIL F
billet * get_n_derniers_billets(fil * f, int n);
//RENVOIE LES N DERNIERS MESSAGES DU FIL DE NUMERO ID, NULL SI ID N'EXISTE PAS A FREE MANUELLEMENT
billet * get_n_derniers_billets_from_id(liste_fils * l ,int id, int n);
//Vérifie que le fichier existe dans les fils
int does_file_exist_fil(fil * f, char * file_name);
//LIBERE LA MEMOIRE OCCUPEE PAR UN FIL F
void free_fil(fil * f);

//CONSTRUIS UNE LISTE VIDE DE FILS
liste_fils * liste_fils_constr();
//AJOUTE UN FIL F A UNE LISTE DE FILS L, RENVOIE LE NUMERO DU NOUVEAU FIL
int ajouter_fil(liste_fils * l, fil * f);
//LIBERE LA MEMOIRE OCCUPEE PAR UNE LISTE DE FILS L
void free_liste_fils(liste_fils * l);

//CONSTRUCTEUR DE NOEUD, NORMALEMENT INTERNE
user_listnode * user_listnode_constr(char * name, int id);
//CONSTRUIS UNE LISTE VIDE D'UTILISATEURS
user_list * user_list_constr();
//TESTE LA PRESENCE D'UN UTILISATEUR DE PSEUDO NAME DANS LA LISTE L, 1 SI SUCCES 0 SINON
int is_in_userlist(user_list * l ,char * name);
//AJOUTE UN UTILISATEUR A LA LISTE D'UTILISATEURS, RENVOIE 1 SI SUCCES 0 SI PSEUDO DEJA UTILISE
int add_user(user_list * l, char * name);
//Renvoie l'identifiant associé au pseudo name si il existe dans la liste, -1 sinon
int get_id(user_list * l, char * name);
//Renvoie le pseudo associé à l'identifiant id si il existe dans la liste, NULL sinon
char * get_name(user_list * l, int id);
//LIBERE LA MEMOIRE OCCUPEE PAR UNE LISTE D'UTILISATEURS
void free_userlist(user_list * l);

#endif