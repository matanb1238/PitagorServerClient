CC=gcc
CFLAGS=-Wall -g

all: thread_pool

client: thread_pool.c
	$(CC) $(CFLAGS) thread_pool.c -o thread_pool

clean:
	rm thread_pool
