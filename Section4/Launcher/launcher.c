#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> // Include for wait
#include <errno.h>
#include "launcher.h"

#define NUM_CLIENTS 5
#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT "8080" // Use string for server port

void launch_client(int client_id) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        char client_id_str[3]; // Buffer to hold the client ID as a string
        snprintf(client_id_str, sizeof(client_id_str), "%d", client_id); // Convert client_id to string

        char *args[] = {
            "./client", // Ensure this path is correct
            SERVER_ADDRESS,
            SERVER_PORT,
            client_id_str, // Pass the client ID as an argument
            NULL
        };

        execvp(args[0], args);
        perror("execvp failed"); // Print error if execvp fails
        exit(EXIT_FAILURE);
    }
}

int main() {
    // Launch multiple clients
    for (int i = 1; i <= NUM_CLIENTS; i++) { // Start client ID from 1
        launch_client(i);
    }

    // Parent process waits for all children
    int status;
    for (int i = 0; i < NUM_CLIENTS; i++) {
        wait(&status); // Wait for each child process
    }

    return 0;
}
