#ifndef CLIENT
#define CLIENT

//Connexion utilisant le protocole IPV4.
int connexion_4();

//Connexion utilisant le protocole IPV6.
int connexion_6();

//Connexion UDP utilisant le protocole IPV4.
int connexion_udp_4(struct sockaddr_in * adrserv, int port);

//Connexion UDP utilisant le protocole IPV6.
int connexion_udp_6(struct sockaddr_in6 * adrserv, int port);

#endif