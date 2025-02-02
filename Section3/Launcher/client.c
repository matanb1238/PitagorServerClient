#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void receive_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received <= 0) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    buffer[bytes_received] = '\0';
    printf("Server response: %s\n", buffer);
}

void send_random_integer(int sockfd, int client_id) {
    unsigned char random_num;
    for (int i = 0; i < 10; i++) {
        random_num = rand() % 5 + 1;
        send(sockfd, &random_num, sizeof(random_num), 0);
        receive_response(sockfd);
        sleep(1); // Prevent spamming the server
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);
    
    send_random_integer(sockfd, 1);

    close(sockfd);
    return 0;
}
