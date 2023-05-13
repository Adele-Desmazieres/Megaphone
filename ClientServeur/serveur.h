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
int connexion_udp(int port);

//Accepte les clients avec en parametre la socket serveur.
int accepter_clients(int sockserv);

//Renvoie une instance de msg_client en fonction de codereq.
msg_client reponse_tcp(char * buf);

//Effectue la communication entre le serveur et le/les clients.
void * communication_client(void * arg);

void envoi_erreur_client(int sockcli);
void envoie_reponse_client(int sockli, msg_serveur reponse_serveur);

int inscription_utili(msg_client * msg_client, user_list * liste_utili);
int poster_billet(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * contenu);
void liste_n_billets();
void abonner_fil();

//Envoie le port du serveur si le fil que l'utilisateur a selectionné existe bien.
int udp_envoi_port_client(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili);

//Recevoit les données du client pour les écrire dans un fichier serveur et créer un billet.
int recevoir_donnees_fichier_serveur(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * file_name);

//Regarde si le fichier existe bien à la fois dans la liste des fils mais aussi physiquement.
int fichier_existe_bdd(msg_client * msg_client, liste_fils * liste_fils);

//Envoie les données du fichier en parametre vers le numéro de port du client qui l'a demandé.
int envoyer_donnees_fichier_serveur(int port, char * file_name);

#endif