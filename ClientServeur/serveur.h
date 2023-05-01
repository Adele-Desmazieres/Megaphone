#ifndef SERVEUR
#define SERVEUR

#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"
#include "bdd_serveur.h"
#include "../UDP/liste_paquets.h"

typedef struct base_serveur {
    user_list * liste_uti;
    liste_fils * liste_fils;
    int socketclient;
} base_serveur;

base_serveur * base_serveur_constr(user_list * ul, liste_fils * lf, int sockli);

//Créé le serveur, renvoie 1 si raté et sinon appelle les autres fonctions de communication.
int creation_serveur();

//Créé un serveur UDP, renvoie 1 si raté.
int connexion_udp(struct sockaddr_in6 sockaddr, int port);

//Accepte les clients avec en parametre la socket serveur.
int accepter_clients(int sockserv);

//Renvoie une instance de msg_client en fonction de codereq.
msg_client reponse_tcp(char * buf);

void * communication_client(void * arg);

void envoi_erreur_client(int sockcli);
void envoie_reponse_client(int sockli, msg_serveur reponse_serveur);

int inscription_utili(msg_client * msg_client, user_list * liste_utili);
int poster_billet(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * contenu);
void liste_n_billets();
void abonner_fil();
int udp_envoi_port_client(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili);
int recevoir_donnees_fichier_serveur(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * file_name);
int fichier_existe_dans_fil(msg_client * msg_client, liste_fils * liste_fils);
int envoyer_donnees_fichier_serveur(int port, char * file_name);

#endif