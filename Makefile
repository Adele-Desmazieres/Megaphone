CC = gcc
CFLAGS = -Wall -pedantic -g -lreadline
HEAD = ClientServeur/client.h ClientServeur/serveur.h ClientServeur/bdd_serveur.h ClientServeur/interpreteur.h MessageStruct/msg_client.h MessageStruct/msg_serveur.h test_struct.c
OBJ_EXEC = ClientServeur/client.o ClientServeur/serveur.o ClientServeur/bdd_serveur.o ClientServeur/interpreteur.o MessageStruct/msg_client.o MessageStruct/msg_serveur.o test_struct.o
EXEC = test_struct

build: $(EXEC) $(OBJ_EXEC)

%.o: %.c $(HEAD)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXEC): $(OBJ_EXEC)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(EXEC)
	rm -f $(OBJ_EXEC)
