CC=gcc
CFLAGS=-Wall -g

all: client launcher server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

launcher: launcher.c
	$(CC) $(CFLAGS) launcher.c -o launcher

server: server.c
	$(CC) $(CFLAGS) server.c -o server	

clean:
	rm client launcher server
