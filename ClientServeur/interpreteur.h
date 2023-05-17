#ifndef INTERPRETEUR
#define INTERPRETEUR

#include "client.h"
#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"
#include "../UDP/liste_paquets.h"

int inscription_ou_debut_session(int *userid);
int interpreteur_utilisateur(int *userid);
int inscription(int *userid);
int debut_session(int *userid);
int lister_commandes_en_session();
int poster_billet_client(int *userid);
int get_n_billets(int userid);

//Envoie un billet avec le nom d'un fichier, ainsi que le contenu du fichier au serveur.
int envoyer_donnees_fichier_client(int *userid);
//Recoit le fichier d'un billet donné et écrit son contenu dans un autre passé en parametres.
int recevoir_donnees_fichier_client(int *userid);
//Envoie un paquet au fichier pour que celui-ci puisse identifier l'adresse du client.
int envoyer_serveur_udp_adr(struct sockaddr_in6 adrserv, int sock);

int abonnement_fil(int userid);
void * thread_notifs(void * args);

#endif