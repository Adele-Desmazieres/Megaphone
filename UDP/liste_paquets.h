#ifndef LISTE_PAQUETS
#define LISTE_PAQUETS

typedef struct paquet paquet;
typedef struct paquet {
    int codereq;
    int id;
    int numbloc;
    char * data;
    paquet * prev;
    paquet * next;
} paquet;

typedef struct liste_paquets liste_paquets;
typedef struct liste_paquets {
    paquet * first;
} liste_paquets;

paquet * paquet_constr(int codereq, int id, int numbloc, char * data, paquet * prev, paquet * next);

//Renvoie la taille d'un msg par rapport à la taille de paquet en accord avec les uint16.
size_t get_taille_msg_udp(paquet paq);

//Transforme les données d'un paquet paq en message uint16.
u_int16_t * paquet_to_udp(size_t len_paq, paquet paq);

//Transforme un message uint16 en un paquet.
paquet * udp_to_paquet(uint16_t * msg);

//Ajoute un paquet dans l'ordre de la liste des paquets en fonction de son numblock.
void push_paquet(liste_paquets * liste, paquet * paq);

//Libère les données d'un paquet.
void free_paquet(paquet * paq);

//Libère la liste des paquets.
void free_liste_paquets(liste_paquets * liste);

//Envoie les données d'un fichier à la sock passée en parametre ainsi que l'adrudp et le port.
int envoyer_donnees_fichier(int sock, struct sockaddr_in6 adrudp, int codereq, int port, char * file_name, int directory_client);

//Recoit les données d'un fichier à la sock passée en parametre.
int recevoir_donnees_fichier(int sock, char * file_name, int directory_client);

//Ecrit les données de tous les paquets dans un fichier.
int ecrire_dans_fichier_udp(char * file_name, liste_paquets * liste_paq, int directory_client);

//En fonction de directory_client renvoie le nom du fichier agrémenté du prefixe "FicClient/" ou "FicServ/"
char * get_directory_file(char * file_name, int directory_client);

#endif