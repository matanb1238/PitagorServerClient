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
#define NUM_MESSAGES 50
#define RANDOM_MAX 17



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

void send_random_integer(int sockfd, unsigned char random_integer)
{
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%hhu", random_integer);
    if (send(sockfd, buffer, strlen(buffer), 0) == -1)
    {
        perror("send");
        exit(EXIT_FAILURE);
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
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <server_address> <server_port> <seed>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_address = argv[1];
    const char *server_port = argv[2];
    const unsigned int seed = atoi(argv[3]);

    printf("Server Address: %s\nServer Port: %s\nSeed: %u\n", 
           server_address, server_port,seed);

    int sockfd = create_connection(server_address, server_port);

    srand(seed);

    for (int i = 0; i < NUM_MESSAGES; i++)
    {
        unsigned int random_num = rand() % RANDOM_MAX + 1;
        printf("Sending random number: %u\n", random_num);
        send_random_integer(sockfd, random_num);

        // Receive response from the server
        receive_response(sockfd);
    }

    // Close the socket
    close(sockfd);

    return 0;
}
