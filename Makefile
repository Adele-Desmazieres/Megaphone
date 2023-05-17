CC = gcc
CFLAGS = -Wall -pedantic -g -lreadline -pthread
HEAD = ClientServeur/interpreteur.h ClientServeur/client.h ClientServeur/serveur.h ClientServeur/bdd_serveur.h ClientServeur/interpreteur.h MessageStruct/msg_client.h MessageStruct/msg_serveur.h UDP/liste_paquets.h UDP/msg_multicast.h
CLIENT_O = ClientServeur/interpreteur.o ClientServeur/client.o MessageStruct/msg_client.o MessageStruct/msg_serveur.o UDP/liste_paquets.o UDP/msg_multicast.o
SERVEUR_O = ClientServeur/serveur.o ClientServeur/bdd_serveur.o MessageStruct/msg_serveur.o MessageStruct/msg_client.o UDP/liste_paquets.o UDP/msg_multicast.o
CLIENT = interpreteur
SERVEUR = serveur

build: $(CLIENT_O) $(SERVEUR_O) $(SERVEUR) $(CLIENT)

%.o: %.c $(HEAD)
	$(CC) -c -o $@ $< $(CFLAGS)

$(CLIENT): $(CLIENT_O)
	$(CC) -o $@ $^ $(CFLAGS)

$(SERVEUR): $(SERVEUR_O)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(CLIENT_O)
	rm -f $(CLIENT)
	rm -f $(SERVEUR_O)
	rm -f $(SERVEUR)