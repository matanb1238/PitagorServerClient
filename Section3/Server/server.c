#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include "server.h"

#define PORT 8080
#define MAX_PENDING_CONNECTIONS 5
#define MAX_REQUESTS 100
#define BUFFER_SIZE 1024

// Function to check if three numbers form a Pythagorean triple
int is_pitagor(unsigned char a, unsigned char b, unsigned char c) {
    return (a * a + b * b == c * c ||
            a * a + c * c == b * b ||
            b * b + c * c == a * a);
}

// Function to log results to a file
void log_to_file(unsigned char *requests, int count, int result) {
    // Open log file and redirect stdout to the file
    int log_fd = open("results.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    dup2(log_fd, STDOUT_FILENO);  // Redirect stdout to the file
    close(log_fd);

    // Write the result to the file
    if (result) {
        printf("YES: The last three edges (%u, %u, %u) form a Pythagorean triple.\n",
               requests[count - 3], requests[count - 2], requests[count - 1]);
    } else {
        printf("NO: The last three edges (%u, %u, %u) do not form a Pythagorean triple.\n",
               requests[count - 3], requests[count - 2], requests[count - 1]);
    }
}

int main() {
    int serverSocketFd;
    struct sockaddr_in serverAddr;
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

    // Set up poll
    struct pollfd fds[MAX_PENDING_CONNECTIONS + 1];
    fds[0].fd = serverSocketFd;
    fds[0].events = POLLIN;

    int client_count = 0;

    // Loop to process client requests
    while (1) {
        // Initialize the fds for polling
        for (int i = 1; i <= client_count; i++) {
            fds[i].fd = fds[i].fd;  // Retain client file descriptors
            fds[i].events = POLLIN;
        }

        // Wait for events on the file descriptors
        int poll_count = poll(fds, client_count + 1, -1);
        if (poll_count < 0) {
            perror("poll failed");
            break;
        }

        // Check for new connections
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientSocketFd = accept(serverSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
            if (clientSocketFd < 0) {
                perror("accept failed");
                continue;
            }

            // Add new client socket to the fds array
            fds[client_count + 1].fd = clientSocketFd;
            fds[client_count + 1].events = POLLIN;
            client_count++;

            printf("Accepted new connection from %s:%d\n",
                   inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        }

        // Handle client requests
        for (int i = 1; i <= client_count; i++) {
            if (fds[i].revents & POLLIN) {
                unsigned char buffer;
                int bytes_received = recv(fds[i].fd, &buffer, sizeof(buffer), 0);
                if (bytes_received <= 0) {
                    if (bytes_received == 0) {
                        printf("Client disconnected\n");
                    } else {
                        perror("Error receiving request");
                    }
                    close(fds[i].fd);
                    fds[i].fd = -1;  // Mark this fd as closed
                } else {
                    printf("Received request from client: %u\n", buffer);

                    // Store the request
                    requests[requests_count % MAX_REQUESTS] = buffer;
                    requests_count++;

                    // Check if we have at least 3 requests
                    if (requests_count >= 3) {
                        // Check if the last 3 requests form a Pythagorean triple
                        unsigned char a = requests[(requests_count - 3) % MAX_REQUESTS];
                        unsigned char b = requests[(requests_count - 2) % MAX_REQUESTS];
                        unsigned char c = requests[(requests_count - 1) % MAX_REQUESTS];
                        int result = is_pitagor(a, b, c);

                        // Log the result to the file
                        log_to_file(requests, requests_count, result);

                        // Send the result to the client
                        const char *response = result ? "YES" : "NO";
                        send(fds[i].fd, response, strlen(response), 0);
                    } else {
                        // Less than 3
                        const char *response = "Not enough samples";
                        send(fds[i].fd, response, strlen(response), 0);
                    }
                }
            }
        }
    }

    // Close the server socket
    close(serverSocketFd);
    return 0;
}
