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
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 10
#define LOG_FILE "pythagorean_log.txt"

// Mutex for log file access and shared data protection
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sides_mutex = PTHREAD_MUTEX_INITIALIZER;
int sides[3] = {0};
int side_count = 0;

// Thread pool management
void thread_pool_init(int num_threads);
void thread_pool_add_task(void (*function)(void *), void *arg);
void thread_pool_destroy();

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

// Function to handle client requests (combining both echo and Pythagorean checking)
void handle_client_request(void *arg) {
    int client_fd = (int)(intptr_t)arg;  // Cast the void* back to int
    char buffer[BUFFER_SIZE];
    char *message;

    while (1) {
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            // Client disconnected or error occurred
            close(client_fd);
            break;
        }

        buffer[bytes_received] = '\0';  // Null-terminate the received data
        printf("Received from client: %s\n", buffer);

        message = strtok(buffer, "\n");  // Extract messages using newline as delimiter
        while (message != NULL) {
            printf("Received message: %s\n", message);
            message = strtok(NULL, "\n");  // Get next message in case of multiple
        }

        // Convert buffer to an integer side
        int side = atoi(buffer);

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

        // Echo the message back to the client (as per the first server's functionality)
        send(client_fd, buffer, bytes_received, 0);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create the socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, THREAD_POOL_SIZE) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Initialize the thread pool
    thread_pool_init(THREAD_POOL_SIZE);

    while (1) {
        // Accept an incoming client connection
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        printf("Accepted a new connection\n");

        // Add the client request as a task to the thread pool
        thread_pool_add_task(handle_client_request, (void *)(intptr_t)client_fd);
    }

    // Clean up resources (this part is unreachable unless the server has a shutdown condition)
    close(server_fd);
    thread_pool_destroy();

    return 0;
}
