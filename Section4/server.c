#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define BACKLOG 10

// Shared variable to store the count of valid right triangles.
int right_triangle_count = 0;
pthread_mutex_t lock;

void *handle_client(void *client_sockfd_ptr) {
    int client_sockfd = *(int *)client_sockfd_ptr;
    free(client_sockfd_ptr);

    char buffer[BUFFER_SIZE];
    int bytes_received;
    unsigned int a, b, c;

    // Receive the sides from the client
    if ((bytes_received = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
        perror("recv");
        close(client_sockfd);
        return NULL;
    }

    buffer[bytes_received] = '\0';  // Null-terminate the received data

    // Parse the sides as integers
    if (sscanf(buffer, "%u %u %u", &a, &b, &c) != 3) {
        char *error_message = "Invalid input format. Please provide three sides.\n";
        send(client_sockfd, error_message, strlen(error_message), 0);
        close(client_sockfd);
        return NULL;
    }

    // Check if a^2 + b^2 = c^2 or its variations
    int is_right_triangle = (a * a + b * b == c * c) ||
                            (a * a + c * c == b * b) ||
                            (b * b + c * c == a * a);

    // Lock before updating the shared count
    pthread_mutex_lock(&lock);
    if (is_right_triangle) {
        right_triangle_count++;
    }
    int current_count = right_triangle_count;  // Store the current count to send to the client.
    pthread_mutex_unlock(&lock);

    // Prepare the response
    snprintf(buffer, BUFFER_SIZE, "Result: %s\nTotal right triangles found: %d\n",
             is_right_triangle ? "YES" : "NO", current_count);

    // Send the response back to the client
    if (send(client_sockfd, buffer, strlen(buffer), 0) == -1) {
        perror("send");
    }

    // Close the client connection
    close(client_sockfd);
    return NULL;
}

int main() {
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    // Create a TCP socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified port
    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sockfd, BACKLOG) == -1) {
        perror("listen");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    // Initialize the mutex
    pthread_mutex_init(&lock, NULL);

    printf("Server listening on port %d...\n", PORT);

    // Accept connections from clients
    while (1) {
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_sockfd == -1) {
            perror("accept");
            continue;
        }

        // Allocate memory for client socket to pass it to the thread
        int *client_sockfd_ptr = malloc(sizeof(int));
        if (client_sockfd_ptr == NULL) {
            perror("malloc");
            close(client_sockfd);
            continue;
        }

        *client_sockfd_ptr = client_sockfd;

        // Create a new thread to handle the client connection
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, client_sockfd_ptr) != 0) {
            perror("pthread_create");
            free(client_sockfd_ptr);
            close(client_sockfd);
            continue;
        }

        // Detach the thread so it cleans up automatically when done
        pthread_detach(client_thread);
    }

    // Cleanup (unreachable in this example since the server runs indefinitely)
    close(server_sockfd);
    pthread_mutex_destroy(&lock);

    return 0;
}
