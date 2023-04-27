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

int poster_fichier_client(int *userid);
int envoyer_donnees_fichier(int *userid, char * file_path, int port);
int recevoir_port_udp();
int poster_fichier_client(int *userid);

#endif