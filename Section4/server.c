#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define PORT 12345
#define MAX_CLIENTS 100
#define BUFFER_SIZE 16
#define LOG_FILE "pythagorean_log.txt"

// Mutex for log file access and shared data protection
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sides_mutex = PTHREAD_MUTEX_INITIALIZER;
int sides[3] = {0};
int side_count = 0;

// Function to check if three numbers form a Pythagorean triple
int is_pythagorean_triple(int a, int b, int c) {
    return (a * a + b * b == c * c) ||
           (b * b + c * c == a * a) ||
           (c * c + a * a == b * b);
}

// Function to log results
void log_result(int a, int b, int c, int result) {
    pthread_mutex_lock(&file_mutex);

    // Get the current time for logging
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove the newline character

    int stdout_copy = dup(STDOUT_FILENO);
    int log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("Error opening log file");
        pthread_mutex_unlock(&file_mutex);
        return;
    }

    dup2(log_fd, STDOUT_FILENO);
    close(log_fd);

    // Log to both the console and file
    printf("[%s] Triangle: (%d, %d, %d) - %s\n", time_str, a, b, c, result ? "Pythagorean Triple" : "Not a Triple");
    fflush(stdout);

    dup2(stdout_copy, STDOUT_FILENO);
    close(stdout_copy);

    pthread_mutex_unlock(&file_mutex);
}

// Thread function to handle client communication
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};

    int bytes_received;
    char *message;
    while (1) {
        while ((bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';  // Null-terminate the received data

            message = strtok(buffer, "\n");  // Extract messages using newline as delimiter
            while (message != NULL) {
                printf("Received: %s\n", message);
                message = strtok(NULL, "\n");  // Get next message in case of multiple
            }

            int side = atoi(buffer);  // Convert buffer to an integer side

            // Update sides array and overwrite the oldest side
            pthread_mutex_lock(&sides_mutex);
            sides[side_count % 3] = side;  // Overwrite the oldest side (circular behavior)
            side_count++;

            // Log the sides each time a new side is received
            printf("Sides: ");
            for (int i = 0; i < 3; i++) {
                printf("%d ", sides[i]);
            }
            printf("\n");

            // After receiving at least 3 sides, check the Pythagorean triple
            if (side_count >= 3) {
                int a = sides[0], b = sides[1], c = sides[2];
                int result = is_pythagorean_triple(a, b, c);
                log_result(a, b, c, result);
            }

            pthread_mutex_unlock(&sides_mutex);
        }
    }

    close(client_fd);
    return NULL;
}

// Main server function
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    // Clear the log file when the server starts
    int log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (log_fd < 0) {
        perror("Error clearing log file");
        exit(EXIT_FAILURE);
    }
    close(log_fd);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Log the time of accepting a new connection
        time_t new_connection_time = time(NULL);
        char *new_connection_str = ctime(&new_connection_time);
        new_connection_str[strlen(new_connection_str) - 1] = '\0'; // Remove the newline character
        //printf("[%s] Accepted new connection\n", new_connection_str);

        // Create thread to handle client
        pthread_t thread_id;
        int *client_fd = malloc(sizeof(int));
        *client_fd = new_socket;
        pthread_create(&thread_id, NULL, handle_client, client_fd);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
