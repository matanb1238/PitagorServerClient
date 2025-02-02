#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define NUM_MESSAGES 20

void send_random_integer(int sockfd) {
    char buffer[16];
    unsigned char random_num;
    for (int i = 0; i < NUM_MESSAGES; i++) {
        random_num = rand() % 5 + 1;
        snprintf(buffer, sizeof(buffer), "%hhu\n", random_num);
        printf("Sending: %s", buffer);
        send(sockfd, buffer, strlen(buffer), 0);
        usleep(100000); // Prevent spamming the server
    }
}


int main() {
    int sock;
    struct sockaddr_in serv_addr;
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // seed by process id
    srand(getpid());

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4/IPv6 address
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    send_random_integer(sock);

    close(sock);
    return 0;
}
