#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include "thread_pool.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 3  // Number of threads in the pool

// Declare the thread pool functions (from the previous implementation)
void thread_pool_init(int num_threads);
void thread_pool_add_task(void (*function)(void *), void *arg);
void thread_pool_destroy();

// Function to handle client requests
void handle_client_request(void *arg) {
    int client_fd = (int)(intptr_t)arg;  // Cast the void* back to int

    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            // Client disconnected or error occurred
            close(client_fd);
            break;
        }

        buffer[bytes_received] = '\0';  // Null-terminate the received data
        printf("Received from client: %s\n", buffer);

        // Echo the message back to the client
        send(client_fd, buffer, bytes_received, 0);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create the socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, THREAD_POOL_SIZE) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Initialize the thread pool
    thread_pool_init(THREAD_POOL_SIZE);

    while (1) {
        // Accept an incoming client connection
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        printf("Accepted a new connection\n");

        // Add the client request as a task to the thread pool
        thread_pool_add_task(handle_client_request, (void *)(intptr_t)client_fd);
    }

    // Clean up resources (this part is unreachable unless the server has a shutdown condition)
    close(server_fd);
    thread_pool_destroy();

    return 0;
}
