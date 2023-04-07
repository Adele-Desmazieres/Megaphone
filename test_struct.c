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
#include "MessageStruct/msg_client.h"
#include "ClientServeur/serveur.h"
#include "ClientServeur/client.h"

int main(int argc, char *argv[]) {
    msg_client message_client;
    message_client.codereq = 1;
    message_client.id = 0;
    message_client.numfil = 5;
    message_client.is_inscript = 1;
    message_client.datalen = 5;
    message_client.data = "abcde";


    return 0;
}

int test_serveur() {
    return 0;
}