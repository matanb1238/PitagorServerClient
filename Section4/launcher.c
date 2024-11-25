#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_CLIENTS 5

void *launch_client(void *arg) {
    system("./client");
    return NULL;
}

int main() {
    pthread_t threads[NUM_CLIENTS];

    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_create(&threads[i], NULL, launch_client, NULL);
    }

    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
