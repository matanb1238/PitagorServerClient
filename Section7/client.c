#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define NUM_CLIENTS 5
#define NUM_EDGES 15

typedef struct ClientArgs {
    int client_id;
} ClientArgs;

// Function for the client thread
void* client_thread(void* arg) {
    ClientArgs* args = (ClientArgs*)arg;
    int client_id = args->client_id;

    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        pthread_exit(NULL);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address or address not supported\n");
        pthread_exit(NULL);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection to server failed for client %d\n", client_id);
        pthread_exit(NULL);
    }

    // Seed random number generator
    srand(time(NULL) + client_id); // Seed with client_id to ensure different sequences for each client

    // Send random edges to server
    for (int i = 0; i < NUM_EDGES; i++) {
        int edge = rand() % 10 + 1;  // Generate random number between 1 and 30
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "%d", edge);
        send(sock, buffer, strlen(buffer), 0);
        printf("Client %d: Sent edge %d\n", client_id, edge);
        sleep(1); // Small delay between sends to simulate real-world behavior
    }

    close(sock);
    free(arg);
    pthread_exit(NULL);
}

int main() {
    pthread_t clients[NUM_CLIENTS];

    // Create and launch client threads
    for (int i = 0; i < NUM_CLIENTS; i++) {
        ClientArgs* args = (ClientArgs*)malloc(sizeof(ClientArgs));
        args->client_id = i + 1;

        pthread_create(&clients[i], NULL, client_thread, (void*)args);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(clients[i], NULL);
    }

    return 0;
}
