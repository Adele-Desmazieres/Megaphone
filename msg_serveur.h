

#include <stdint.h>

typedef struct msg_serveur {
    int codereq, id, numfil, nb;
} msg_serveur;

msg_serveur * msg_serveur_constr(int codereq, int id, int numfil, int nb);
uint16_t * msg_serveur_to_send(msg_serveur struc);
msg_serveur * tcp_to_msgserveur(uint16_t * msg);