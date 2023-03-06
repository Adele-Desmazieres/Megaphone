CC = gcc
CFLAGS = -Wall -pedantic -g -lreadline
HEAD = msg_client.h test_struct.c client.c serveur.c
OBJ_EXEC = msg_client.o test_struct.o client.o serveur.o
EXEC = test_struct

build: $(EXEC) $(OBJ_EXEC)

%.o: %.c $(HEAD)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXEC): $(OBJ_EXEC)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(EXEC)
	rm -f $(OBJ_EXEC)
