CC=gcc
CFLAGS=-Wall -g

all: client launcher

client: client.c
	$(CC) $(CFLAGS) client.c -o client

launcher: launcher.c
	$(CC) $(CFLAGS) launcher.c -o launcher

clean:
	rm client launcher
