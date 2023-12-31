
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "bdd_serveur.h"

/*
    TOUTES LES CHAINES DE CARACTERES SONT PASSEES PAR COPIES AUX STRUCTURES, 
    CA VEUT DIRE QU'ELLES SONT MALLOC'd A CHAQUE AJOUT ET QU'ELLES SONT LIBEREES EN UTILISANT 
    LES FONCTIONS DE FREE ICI.
*/


//CONSTRUIS UN BILLET, THEORIQUEMENT INTERNE

billet * billet_constr(char * auteur, char * texte, int is_new_for_notif){

    billet * ret = malloc(sizeof(billet));
    if(ret == NULL) perror("malloc billet");
    ret->suiv = NULL;
    ret->prec = NULL;
    ret->is_new = is_new_for_notif;

    ret->texte = malloc((strlen(texte) + 1) * sizeof(char));
    if(ret->texte == NULL) perror("malloc billet");
    memset(ret -> texte, '\0', strlen(texte) + 1);
    strncpy(ret -> texte, texte, strlen(texte));

    ret->auteur = malloc((strlen(auteur) + 1) * sizeof(char));
    if(ret->auteur == NULL) perror("malloc billet");
    memset(ret -> auteur, '\0', strlen(auteur) + 1);
    strncpy(ret -> auteur, auteur, strlen(auteur));

    return ret;

}

//CONSTRUIS UN FIL A PARTIR D'UN AUTEUR ET UN TEXTE

fil * fil_constr(char * auteur, char * texte){

    fil * ret = malloc(sizeof(fil));
    if(ret == NULL) perror("malloc fil");

    
    ret->premier_msg = billet_constr(auteur, texte, 0);
    ret->nb_de_msg = 1;

    ret->suiv = NULL;

    ret->is_multicast = 0;
    ret->multicast_sockfd = -1;
    ret->multicast_addr = NULL;
    ret->sockopt = NULL;

    return ret;

}

//Récupère le fil par le numéro de fil passé en parametre, renvoie NULL si non trouvé.
fil * get_fil_id(liste_fils * l, int numfil) {

    int index = 1;
    fil * current = l -> premier_fil;
    while (index != numfil && current -> suiv != NULL) {
        index += 1;
        current = current -> suiv;
    }

    if (index == numfil) {
        return current;
    }

    return NULL;
}

//CONSTRUIS UNE LISTE VIDE DE FILS
liste_fils * liste_fils_constr(){

    liste_fils * ret = malloc(sizeof(liste_fils));
    if(ret == NULL) perror("Erreur liste fils");

    ret->nb_de_fils = 0;
    ret->premier_fil = NULL;

    return ret;

}

//AJOUTE UN FIL F A UNE LISTE DE FILS L, RENVOIE LE NUMERO DU NOUVEAU FIL
int ajouter_fil(liste_fils * l, fil * f) {

    if(l->premier_fil == NULL){
        l->premier_fil = f;
        f->id = l->nb_de_fils;
        l->nb_de_fils++;
        return 1;
    }

    int index = 1;
    fil * tmp = l->premier_fil;
    while(tmp->suiv != NULL) { index += 1; tmp = tmp->suiv; }

    tmp->suiv = f;
    f->id = l->nb_de_fils;
    l->nb_de_fils ++;

    return index + 1;

}

//AJOUTE UN BILLET (AUTEUR, TEXTE) A UN FIL F
void ajouter_billet(fil * f, char * auteur, char * texte, int is_new_for_notif){

    billet * billet_tmp = f->premier_msg;

    while(billet_tmp->suiv != NULL) billet_tmp = billet_tmp->suiv;

    billet * nouv = billet_constr(auteur, texte, is_new_for_notif);

    billet_tmp->suiv = nouv;
    nouv->prec = billet_tmp;

    f->nb_de_msg++;

    return;

}

//RENVOIE LES N DERNIERS BILLET DU FIL F
billet * get_n_derniers_billets(fil * f, int n){

    if(n > f->nb_de_msg) n = f->nb_de_msg;

    billet * ret = malloc(n * sizeof(billet));
    if(ret == NULL) perror("malloc get n billets\n");

    billet * billet_tmp = f->premier_msg;
    while(billet_tmp->suiv != NULL) billet_tmp = billet_tmp->suiv;

    int acc = 0;

    while(acc < n){
        ret[acc] = *billet_tmp;
        acc++;
        billet_tmp = billet_tmp->prec;
    }

    return ret;
}

//RENVOIE LES N DERNIERS MESSAGES DU FIL DE NUMERO ID, NULL SI ID N'EXISTE PAS
//A FREE MANUELLEMENT
billet * get_n_derniers_billets_from_id(liste_fils * l ,int id, int n){

    fil * tmp = l->premier_fil;
    while(tmp != NULL && tmp->id != id) tmp = tmp->suiv;

    if(tmp == NULL) return NULL;

    return get_n_derniers_billets(tmp, n);

}

//Vérifie que le fichier existe dans les fils
int does_file_exist_fil(fil * f, char * file_name) {
    billet * courant = f -> premier_msg;
    while(courant != NULL) {
        if (strncmp(courant -> texte, file_name, strlen(file_name)) == 0) return 0;
        courant = courant -> suiv;
    }

    return -1;
}

//LIBERE LA MEMOIRE OCCUPEE PAR UN FIL F
void free_fil(fil * f){

    billet * tmp = f->premier_msg;
    while (tmp != NULL)
    {
        free(tmp->auteur);
        free(tmp->texte);

        billet * tmp2 = tmp;
        tmp = tmp->suiv;
        free(tmp2);
    }

    free(f);
    

}

//LIBERE LA MEMOIRE OCCUPEE PAR UNE LISTE DE FILS L
void free_liste_fils(liste_fils * l){

    fil * tmp = l->premier_fil;
    while(tmp != NULL){

        fil * tmp2 = tmp;
        tmp = tmp->suiv;
        free(tmp2->multicast_addr);
        free_fil(tmp2);

    }

    free(l);

}



//CONSTRUCTEUR DE NOEUD, NORMALEMENT INTERNE
user_listnode * user_listnode_constr(char * name, int id){

    user_listnode * ret = malloc(sizeof(user_listnode));
    if(ret == NULL) perror("malloc userlist_node");

    ret->id = id;
    ret->pseudo = malloc((strlen(name) +1) * sizeof(char));
    if(ret->pseudo == NULL) perror("malloc userlist_node");
    memset(ret -> pseudo, '\0', strlen(name) + 1);
    strncpy(ret -> pseudo, name, strlen(name));

    ret->suiv = NULL;

    return ret;

}

//CONSTRUIS UNE LISTE VIDE D'UTILISATEURS
user_list * user_list_constr(){

    user_list * ret = malloc(sizeof(user_list));
    if (ret == NULL) perror("malloc user_list");

    ret->first = NULL;
    ret->len = 0;

    return ret;

}

//TESTE LA PRESENCE D'UN UTILISATEUR DE PSEUDO NAME DANS LA LISTE L, 1 SI SUCCES 0 SINON
int is_in_userlist(user_list * l ,char * name){


    user_listnode * tmp = l->first;

    while(tmp != NULL){

        if(strcmp(tmp->pseudo, name) == 0) return 1;
        tmp = tmp->suiv;

    }

    return 0;

}

//AJOUTE UN UTILISATEUR A LA LISTE D'UTILISATEURS, RENVOIE 1 SI SUCCES 0 SI PSEUDO DEJA UTILISE
int add_user(user_list * l, char * name){

    if(is_in_userlist(l, name)) return 0;


    user_listnode * newuser = user_listnode_constr(name, l->len);

    if(l->first == NULL){

        l->first = newuser;
        l->len++;
        return 1;

    }

    user_listnode * tmp = l->first;
    while(tmp->suiv != NULL) tmp = tmp->suiv;

    tmp->suiv = newuser;
    l->len++;
    return 1;

}

//Renvoie l'identifiant associé au pseudo name si il existe dans la liste, -1 sinon
int get_id(user_list * l, char * name){

    for(user_listnode * tmp = l->first; tmp != NULL; tmp = tmp->suiv){

        if(strcmp(tmp->pseudo, name) == 0) return tmp->id;

    }

    return -1;
}

//Renvoie le pseudo associé à l'identifiant id si il existe dans la liste, NULL sinon
char * get_name(user_list * l, int id){

    for(user_listnode * tmp = l->first; tmp != NULL; tmp = tmp->suiv){

        if(tmp->id == id) return tmp->pseudo;

    }

    return NULL;

}

//LIBERE LA MEMOIRE OCCUPEE PAR UNE LISTE D'UTILISATEURS
void free_userlist(user_list * l){

    user_listnode * tmp = l->first;
    while(tmp != NULL){

        free(tmp->pseudo);
        user_listnode * tmp2 = tmp;
        tmp = tmp->suiv;
        free(tmp2);

    }

    free(l);

}