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

//Constructeur pour la base de donnée serveur
base_serveur * base_serveur_constr(user_list * ul, liste_fils * lf, int sockli);
//Créé le serveur, renvoie 1 si raté et sinon appelle les autres fonctions de communication.
int creation_serveur();
//Créé un serveur UDP, renvoie 1 si raté.
int connexion_udp(int port);

//Accepte les clients avec en parametre la socket serveur.
int accepter_clients(int sockserv);
//Effectue la communication entre le serveur et le/les clients.
void * communication_client(void * arg);
//Envoie un message d'erreur avec pour code 31 au client.
void envoi_erreur_client(int sockcli);
//Transforme un objet msg_serveur en uint16 puis l'envoie au client.
void envoie_reponse_client(int sockli, msg_serveur reponse_serveur);

//Permet à l'utilisateur de s'inscrire. Renvoie l'identifiant de l'utilisateur en cas de succès.
int inscription_utili(msg_client * msg_client, user_list * liste_utili);
//Permet de poster un billet dans un fil passé en parametre du message. Renvoie le numéro 
//du fil ou le billet a été posté en cas de succès.
int poster_billet(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * contenu);
//Renvoie la liste des n billets en fonctions des options de l'utilisateur.
void liste_n_billets(int sockcli, liste_fils * liste_fils, msg_client * msg_client);
//Abonne un utilisateur à un fil et lui envoie des notifications lorsqu'il y a un nouveau post.
void abonner_fil(fil * f, msg_client * msg_client, int sockcli, base_serveur * bs);
//Envoie le port du serveur si le fil que l'utilisateur a selectionné existe bien.
int udp_envoi_port_client(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili);
//Reçoit les données du client pour les écrire dans un fichier serveur et créer un billet.
int recevoir_donnees_fichier_serveur(msg_client * msg_client, liste_fils * liste_fils, user_list * liste_utili, char * file_name);
//Regarde si le fichier existe bien à la fois dans la liste des fils mais aussi physiquement.
int fichier_existe_bdd(msg_client * msg_client, liste_fils * liste_fils);
//Envoie les données du fichier en parametre vers le numéro de port du client qui l'a demandé.
int envoyer_donnees_fichier_serveur(int port, char * file_name);
//Gère l'envoi des notifs multicast sur un temps défini
void * gestion_notifications(void * arg);

#endif