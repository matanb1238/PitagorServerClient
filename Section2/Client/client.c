#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

void send_random_integer(int sockfd)
{
    unsigned char random_num;
    for (int i = 0; i < 10; i++) {
        // Generate a random unsigned char
        random_num = rand() % 255 + 1;
        printf("Sending: %d\n", random_num);

        // Send the random number to the server
        send(sockfd, &random_num, sizeof(random_num), 0);

        // Recieve response from the server
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
    printf("Received: %s\n"
            "========\n", buffer);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <server_address> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_address = argv[1];
    const char *server_port = argv[2];
    const unsigned int seed = atoi(argv[3]);

    printf("Server Address: %s\n, Server Port: %s\n, Seed: %u\n", server_address, server_port, seed);

    int sockfd = create_connection(server_address, server_port);

    srand(seed);

    send_random_integer(sockfd);

    // Close the socket
    close(sockfd);

    return 0;
}
