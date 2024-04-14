#ifndef CLIENT_H
#define CLIENT_H

#define BUFFER_SIZE 1024

int create_connection(const char *server_address, const char *server_port);

void send_random_integer(int sockfd, unsigned char random_integer);

void receive_response(int sockfd);

#endif /* CLIENT_H */
