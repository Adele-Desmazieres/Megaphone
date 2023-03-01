#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stdint.h>

#include "test_struct.h"
#include "msg_client.h"

int main(int argc, char *argv[]) {
    msg_client message_client;
    message_client.codereq = 1;
    message_client.id = 0;
    message_client.numfil = 5;
    message_client.is_inscript = 1;
    message_client.datalen = 5;
    message_client.data = "abcde";

    uint16_t * msgclient_to_tcp = msg_client_to_send(message_client);
    msg_client * message_client_from_tcp = tcp_to_msgclient(msgclient_to_tcp);

    printf("codereq = %d \n",message_client_from_tcp->codereq);
    printf("ID = %d \n",message_client_from_tcp->id);
    printf("data = %s \n",message_client_from_tcp->data);
    printf("numfil = %d\n", message_client_from_tcp->numfil);



    return 0;
}