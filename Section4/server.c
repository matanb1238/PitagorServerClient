#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

pthread_mutex_t lock;

void write_result_to_file(unsigned int a, unsigned int b, unsigned int c, int is_pythagorean) {
    pthread_mutex_lock(&lock);

    int file_fd = open("triangle_results.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd == -1) {
        perror("open");
        pthread_mutex_unlock(&lock);
        return;
    }

    int stdout_backup = dup(STDOUT_FILENO);
    dup2(file_fd, STDOUT_FILENO);

    // Debugging output to confirm writing
    printf("Writing to file: Sides: %u %u %u - %s\n", a, b, c, is_pythagorean ? "YES" : "NO");

    dup2(stdout_backup, STDOUT_FILENO);
    close(stdout_backup);
    close(file_fd);

    pthread_mutex_unlock(&lock);
}

void check_triangle_and_write(const char *buffer) {
    unsigned int a, b, c;
    if (sscanf(buffer, "%u %u %u", &a, &b, &c) == 3) {
        int is_pythagorean = (a * a + b * b == c * c) ||
                             (a * a + c * c == b * b) ||
                             (b * b + c * c == a * a);
        write_result_to_file(a, b, c, is_pythagorean);
    } else {
        fprintf(stderr, "Error parsing message: %s\n", buffer);
    }
}

void *poll_for_clients(void *arg) {
    int server_fd = *(int *)arg;
    struct pollfd client_fds[MAX_CLIENTS];
    int client_count = 0;

    client_fds[0].fd = server_fd;
    client_fds[0].events = POLLIN;

    while (1) {
        int poll_count = poll(client_fds, client_count + 1, -1);
        if (poll_count == -1) {
            perror("poll");
            continue;
        }

        for (int i = 0; i <= client_count; i++) {
            if (client_fds[i].revents & POLLIN) {
                if (client_fds[i].fd == server_fd) {
                    int client_fd = accept(server_fd, NULL, NULL);
                    if (client_fd == -1) {
                        perror("accept");
                    } else {
                        printf("New client connected: FD %d\n", client_fd);
                        if (client_count < MAX_CLIENTS) {
                            client_fds[++client_count].fd = client_fd;
                            client_fds[client_count].events = POLLIN;
                        } else {
                            perror("Max clients reached");
                            close(client_fd);
                        }
                    }
                } else {
                    char buffer[BUFFER_SIZE];
                    int bytes_received = recv(client_fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    if (bytes_received <= 0) {
                        close(client_fds[i].fd);
                        client_fds[i] = client_fds[client_count--];
                        printf("Client disconnected: FD %d\n", client_fds[i].fd);
                    } else {
                        buffer[bytes_received] = '\0';
                        printf("Received message: %s\n", buffer);
                        check_triangle_and_write(buffer);
                    }
                }
            }
        }
    }
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&lock, NULL);

    pthread_t poll_thread;
    pthread_create(&poll_thread, NULL, poll_for_clients, &server_fd);
    pthread_join(poll_thread, NULL);

    pthread_mutex_destroy(&lock);
    close(server_fd);
    return 0;
}
