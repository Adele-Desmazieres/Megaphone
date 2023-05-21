#ifndef INTERPRETEUR
#define INTERPRETEUR

#include "client.h"
#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"
#include "../UDP/liste_paquets.h"


//Arrete le progamme client, free les pointeurs.
int arret(int *userid);
//Page d'accueil lors du lancement du client. L'utilisateur peut choisir de s'inscrire ou de démarrer sa session.
int inscription_ou_debut_session(int *userid);
//Interpreteur du côté utilisateur
int interpreteur_utilisateur(int *userid);
//Inscrit l'utilisateur : - en cas de réussite, initialise userid et renvoie 0 - sinon renvoie -1
int inscription(int *userid);
//Démarrage d'une session utilisateur avec son identifiant.
//Ne fait aucune vérif quant à l'existence de l'identifiant dans les données du serveur. Renvoie 0 si réussite, et -1 sinon.
int debut_session(int *userid);

//Affiche les commandes accessibles lorsqu'un utilisateur est en cours de session.
int lister_commandes_en_session();
//Crée un billet et l'envoie au serveur
int poster_billet_client(int *userid);
//Récupère la liste des n billets en fonction des options rentrées par l'utilisateur.
int get_n_billets(int userid);
//S'abonne à un fil dont le numéro est rentré par l'utilisateur et recoit des notifs à chaque post.
int abonnement_fil(int userid);
//Envoie des notifications à l'utilisateur lorsque il y a un nouveau post au fil auquel iel est abonné.e.
void * thread_notifs(void * args);
//Envoie un billet avec le nom d'un fichier, ainsi que le contenu du fichier au serveur.
int envoyer_donnees_fichier_client(int *userid);
//Recoit le fichier d'un billet donné et écrit son contenu dans un autre passé en parametres.
int recevoir_donnees_fichier_client(int *userid);
//Envoie un paquet au fichier pour que celui-ci puisse identifier l'adresse du client.
int envoyer_serveur_udp_adr(struct sockaddr_in6 adrserv, int sock);

#endif