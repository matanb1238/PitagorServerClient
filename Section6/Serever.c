#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1" // Localhost
#define MESSAGE "Hello, Server!"

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    // Send message to the server
    if (send(sock_fd, MESSAGE, strlen(MESSAGE), 0) == -1) {
        perror("send");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Message sent to server: %s\n", MESSAGE);

    // Receive response from the server
    int bytes_received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received == -1) {
        perror("recv");
    } else if (bytes_received == 0) {
        printf("Server closed the connection.\n");
    } else {
        buffer[bytes_received] = '\0';
        printf("Message received from server: %s\n", buffer);
    }

    // Close the connection
    close(sock_fd);

    return 0;
}
