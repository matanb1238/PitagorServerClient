CC = gcc
CFLAGS = -Wall -g -pthread

all: client launcher server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

launcher: launcher.c
	$(CC) $(CFLAGS) launcher.c -o launcher

server: server.c ../Section6/thread_pool.c
	$(CC) $(CFLAGS) server.c ../Section6/thread_pool.c -o server

clean:
	rm -f client launcher server
