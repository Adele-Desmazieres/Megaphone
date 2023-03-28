#ifndef INTERPRETEUR
#define INTERPRETEUR

int inscription_ou_debut_session(int *userid);
int interpreteur_utilisateur(int *userid);
int inscription(int *userid);
int debut_session(int *userid);
int lister_commandes_en_session();
int poster_billet_client(int *userid);

#endif