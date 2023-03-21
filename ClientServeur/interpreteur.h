#include "client.h"
#include "../MessageStruct/msg_client.h"
#include "../MessageStruct/msg_serveur.h"

int inscription_ou_connexion(int *userid);
int interpreteur_utilisateur(int *userid);
int inscription(int *userid);
int poster_billet_client(int *userid);
