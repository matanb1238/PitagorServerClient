#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>

#define MAX_REQUESTS 100
#define BUFFER_SIZE 1024

unsigned char requests[MAX_REQUESTS];
int requests_count = 0;

int is_pitagor(unsigned char a, unsigned char b, unsigned char c) {
    return (a * a + b * b == c * c);
}

void log_to_file(unsigned char *requests, int count, int result) {
    int log_fd = open("results.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("Failed to open log file");
        return;
    }

    int stdout_fd = dup(STDOUT_FILENO); // Save original stdout
    if (stdout_fd < 0) {
        perror("Failed to duplicate stdout");
        close(log_fd);
        return;
    }

    dup2(log_fd, STDOUT_FILENO);
    close(log_fd);

    // Write the result to the file
    if (result) {
        printf("YES: The last three edges (%u, %u, %u) form a Pythagorean triple.\n",
               requests[count - 3], requests[count - 2], requests[count - 1]);
    } else {
        printf("NO: The last three edges (%u, %u, %u) do not form a Pythagorean triple.\n",
               requests[count - 3], requests[count - 2], requests[count - 1]);
    }

    fflush(stdout);
    dup2(stdout_fd, STDOUT_FILENO); // Restore original stdout
    close(stdout_fd);
}

int main() {
    int sockfd, new_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct pollfd fds[10];
    int client_count = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    printf("Server started. Waiting for clients...\n");

    while (1) {
        int poll_count = poll(fds, client_count + 1, -1);
        if (poll_count < 0) {
            perror("Poll failed");
            continue;
        }

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

                    // Remove client from the array
                    for (int j = i; j < client_count; j++) {
                        fds[j] = fds[j + 1];
                    }
                    client_count--;
                    i--; // Adjust index
                    continue;
                }
                printf("Received request from client %d: %u\n", i, buffer);
                requests[requests_count % MAX_REQUESTS] = buffer;
                requests_count++;

                if (requests_count >= 3) {
                    unsigned char a = requests[(requests_count - 3 + MAX_REQUESTS) % MAX_REQUESTS];
                    unsigned char b = requests[(requests_count - 2 + MAX_REQUESTS) % MAX_REQUESTS];
                    unsigned char c = requests[(requests_count - 1 + MAX_REQUESTS) % MAX_REQUESTS];
                    int result = is_pitagor(a, b, c);

                    log_to_file(requests, requests_count, result);

                    const char *response = result ? "YES" : "NO";
                    send(fds[i].fd, response, strlen(response), 0);
                } else {
                    const char *response = "Not enough samples";
                    send(fds[i].fd, response, strlen(response), 0);
                }
            }
        }

        // Accept new client connections
        if (fds[0].revents & POLLIN) {
            new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (new_fd < 0) {
                perror("Accept failed");
                continue;
            }

            fds[client_count + 1].fd = new_fd;
            fds[client_count + 1].events = POLLIN;
            client_count++;
            printf("New client connected\n");
        }
    }

    close(sockfd);
    return 0;
}
