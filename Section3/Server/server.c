#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "server.h"

#define PORT 8080
#define MAX_PENDING_CONNECTIONS 5
#define MAX_REQUESTS 3
#define BUFFER_SIZE 1024


int is_pitagor(unsigned char a, unsigned char b, unsigned char c){
    if (a * a + b * b == c * c ||
        a * a + c * c == b * b ||
        b * b + c * c == a * a) {
            return 1;
        }
    return 0;
}

int main() {
    int serverSocketFd, clientSocketFd;
    struct sockaddr_in serverAddr, clientAddr;
    unsigned char requests[MAX_REQUESTS];
    int requests_count = 0;

    // Create a socket
    if ((serverSocketFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(serverSocketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocketFd, MAX_PENDING_CONNECTIONS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Accept client connection
        socklen_t clientAddrLen = sizeof(clientAddr);
        clientSocketFd = accept(serverSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocketFd < 0) {
            perror("accept failed");
            continue; // Retry accepting a new client
        }

        printf("Accepted new connection from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        
        requests_count = 0;

        // Loop to process client requests
        while (1) {
            char buffer[BUFFER_SIZE];
            int bytes_received = recv(clientSocketFd, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    printf("Client disconnected\n");
                } else {
                    perror("Error receiving request");
                }
                close(clientSocketFd);
                break;
            }

            printf("Bytes Received = %d\n", bytes_received);
            buffer[bytes_received] = '\0';

            // Convert received data to unsigned integer
            unsigned int request = (unsigned char)buffer[0];
            printf("Received request from client: %u\n", request);

            // Store the request
            requests[requests_count % MAX_REQUESTS] = request;
            requests_count++;

            // Check if we have at least 3 requests
            if (requests_count >= MAX_REQUESTS) {
                unsigned char a = requests[0];
                unsigned char b = requests[1];
                unsigned char c = requests[2];
                const char *response = is_pitagor(a, b, c) ? "YES" : "NO";
                send(clientSocketFd, response, strlen(response), 0);
                printf("Sent response to client: %s\n", response);
            } else {
                const char *response = "Not enough samples";
                send(clientSocketFd, response, strlen(response), 0);
                printf("Sent response to client: %s\n", response);
            }
        }

        // Close the client socket and loop back to accept a new client connection
    }

    // Close the server socket when done (if we ever break out of the main loop)
    close(serverSocketFd);
    return 0;
}
