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
u_int16_t * paquet_to_udp(paquet paq);
paquet * udp_to_paquet(uint16_t * msg);
paquet * pop_paquet(liste_paquets * liste);
void push_paquet(liste_paquets * liste, paquet * paq);
void free_paquet(paquet * paq);
void free_liste_paquets(liste_paquets * liste);
int ecrire_dans_fichier_udp(char * file_name, liste_paquets * liste_paq);

#endif