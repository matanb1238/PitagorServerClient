#ifndef SERVER_H
#define SERVER_H

void handle_client_request(int clientSocketFd, unsigned char *requests, int *requests_count);

#endif /* SERVER_H */
