#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

void connexion() {
    int sock = socket(PF_INET6, SOCK_STREAM, 0);
    
}