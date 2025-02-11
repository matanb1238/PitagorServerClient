#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include "server.h"

#define PORT 8080
#define MAX_PENDING_CONNECTIONS 5
#define MAX_REQUESTS 3
#define BUFFER_SIZE 1024

void handle_client_request(int clientSocketFd, unsigned char *requests, int *requests_count) {
    while (1) { // Keep the connection open for multiple requests
        // Receive the client's request
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer)); // Initialize buffer to all zeros
        int bytes_received = recv(clientSocketFd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Client disconnected\n");
            } else {
                perror("Error receiving request");
            }
            close(clientSocketFd);
            break; // Exit the loop if the connection is closed
        }

        buffer[bytes_received] = '\0'; // Null-terminate the string
        unsigned int request = atoi(buffer);
        printf("Received request from client: %u\n", request);

        // Store the request
        requests[*requests_count % MAX_REQUESTS] = request;
        (*requests_count)++;

        // Check if we have at least 3 requests
        if (*requests_count >= MAX_REQUESTS) {
            // Check if the last 3 requests form a Pythagorean triple
            unsigned char a = requests[(*requests_count - 3) % MAX_REQUESTS];
            unsigned char b = requests[(*requests_count - 2) % MAX_REQUESTS];
            unsigned char c = requests[(*requests_count - 1) % MAX_REQUESTS];
            printf("Checking %u %u %u \n",a,b,c);
            if (a * a + b * b == c * c ||
                a * a + c * c == b * b ||
                b * b + c * c == a * a) {
                // Respond with YES
                const char *response = "YES";
                send(clientSocketFd, response, strlen(response), 0);
                printf("Sent response to client: %s\n", response);
            } else {
                // Respond with NO
                const char *response = "NO";
                send(clientSocketFd, response, strlen(response), 0);
                printf("Sent response to client: %s\n", response);
            }
        } else {
            // Less than 3 requests
            const char *response = "Not enough samples";
            send(clientSocketFd, response, strlen(response), 0);
            printf("Sent response to client: %s\n", response);
        }
    }
}

int main() {
    int serverSocketFd, clientSocketFd;
    struct sockaddr_in serverAddr, clientAddr;
    unsigned char requests[MAX_REQUESTS];
    int requests_count = 0;
    
    // Create a socket
    if (serverSocketFd = socket(AF_INET, SOCK_STREAM, 0) == 0) {
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

    // Accept incoming connections
    while (1) {
        socklen_t clientAddrLen = sizeof(clientAddr);
        if ((clientSocketFd = accept(serverSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
            perror("accept failed");
            continue;
        }

        printf("Accepted new connection from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        // Handle client requests (multiple requests from the same client)
        handle_client_request(clientSocketFd, requests, &requests_count);
    }

    // Close the server socket
    close(serverSocketFd);

    return 0;
}
