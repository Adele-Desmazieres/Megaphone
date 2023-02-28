#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <readline/readline.h>
#include <stdbool.h>

#include "test_struct.h"
#include "msg_client.h"

int main(int argc, char *argv[]) {
    msg_client message_client;
    message_client.codereq = 2;
    message_client.id = 2;
    message_client.numfil = 5;
    message_client.is_inscript = false;
    message_client.datalen = 9;
    message_client.data = "monpseudo";

    char ** msgclient_to_tcp = msg_client_to_send(message_client);
    //msg_client * message_client_from_tcp = tcp_to_msgclient(msgclient_to_tcp);

    return 0;
}