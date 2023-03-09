#include "msg_client.h"

//Créé le serveur, renvoie 1 si raté et sinon appelle les autres fonctions de communication.
int creation_serveur();

//Accepte les clients avec en parametre la socket serveur.
int accepter_clients(int sockserv);

//Renvoie une instance de msg_client en fonction de codereq.
msg_client reponse_tcp(char * buf);

void * communication_client(void * arg);