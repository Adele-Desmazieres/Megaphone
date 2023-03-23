#ifndef SERVEUR
#define SERVEUR

#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"
#include "bdd_serveur.h"

//Créé le serveur, renvoie 1 si raté et sinon appelle les autres fonctions de communication.
int creation_serveur();

//Accepte les clients avec en parametre la socket serveur.
int accepter_clients(int sockserv);

//Renvoie une instance de msg_client en fonction de codereq.
msg_client reponse_tcp(char * buf);

void * communication_client(void * arg);

void envoi_erreur_client(int sockcli);
void envoie_reponse_client(int sockli, msg_serveur reponse_serveur);

int inscription_utili(msg_client * msg_client, user_list * liste_utili);
int poster_billet(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili);
void liste_n_billets();
void abonner_fil();
void ajouter_fichier();
void telecharger_fichier();

#endif