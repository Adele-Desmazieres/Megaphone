
//STRUCTURE BILLET
typedef struct billet billet;
typedef struct billet {
    char * texte;
    char * auteur;
    billet * prec;
    billet * suiv;
} billet;

//STRUCTURE FIL
typedef struct fil fil;
typedef struct fil {

    int id;
    int nb_de_msg;
    billet * premier_msg;

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

billet * billet_constr(char * auteur, char * texte);

fil * fil_constr(char * auteur, char * texte);
fil * get_fil_id(liste_fils * l, int numfil);
void ajouter_billet(fil * f, char * auteur, char * texte);
billet * get_n_derniers_billets(fil * f, int n);
billet * get_n_derniers_billets_from_id(liste_fils * l ,int id, int n);
void free_fil(fil * f);

liste_fils * liste_fils_constr();
int ajouter_fil(liste_fils * l, fil * f);
void free_liste_fils(liste_fils * l);

user_listnode * user_listnode_constr(char * name, int id);
user_list * user_list_constr();
int is_in_userlist(user_list * l ,char * name);
int add_user(user_list * l, char * name);
int get_id(user_list * l, char * name);
char * get_name(user_list * l, int id);
void free_userlist(user_list * l);