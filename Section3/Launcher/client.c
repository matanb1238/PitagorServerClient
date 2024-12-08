#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <arpa/inet.h>
#include "client.h"

#define BUFFER_SIZE 1024

int create_connection(const char *server_address, const char *server_port)
{
    struct addrinfo hints, *res, *p;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_address, server_port, &hints, &res) != 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1)
            break;

        close(sockfd);
    }

    freeaddrinfo(res);

    if (p == NULL)
    {
        fprintf(stderr, "Failed to connect\n");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void send_random_integer(int sockfd, int client_id)
{
    unsigned char random_num;
    for (int i = 0; i < 10; i++) {
        // Generate a random unsigned char
        random_num = rand() % 256 + 1;
        printf("Client %d Sending: %d\n", client_id, random_num); // Include client ID

        // Send the random number to the server
        send(sockfd, &random_num, sizeof(random_num), 0);

        // Receive response from the server
        receive_response(sockfd);
    }
}

void receive_response(int sockfd)
{
    char buffer[BUFFER_SIZE];
    int bytes_received;

    if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    buffer[bytes_received] = '\0';
    printf("Received: %s\n", buffer);
}

int main(int argc, char *argv[])
{
    if (argc != 4) // Ensure to include client ID
    {
        fprintf(stderr, "Usage: %s <server_address> <server_port> <client_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_address = argv[1];
    const char *server_port = argv[2];
    int client_id = atoi(argv[3]); // Get client ID from arguments

    printf("Client %d: Server Address: %s, Server Port: %s\n", client_id, server_address, server_port);

    int sockfd = create_connection(server_address, server_port);

    srand(time(NULL) ^ (client_id << 16)); // Seed random with client ID for variability

    send_random_integer(sockfd, client_id); // Pass client ID to function

    // Close the socket
    close(sockfd);

    return 0;
}
