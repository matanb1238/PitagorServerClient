#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include "../Section6/thread_pool.h"  // Include the thread pool header

#define PORT 8080
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 10
#define LOG_FILE "results.txt"

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sides_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t report_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t report_cond = PTHREAD_COND_INITIALIZER;

int sides[3] = {0};
int side_count = 0;
int total_checked = 0;
int pythagorean_count = 0;
int reporting_started = 0;
int server_fd;
int running = 1;  // Control flag for the main loop

// Function to check if three numbers form a Pythagorean triple
int is_pythagorean_triple(int a, int b, int c) {
    return (a * a + b * b == c * c) ||
           (b * b + c * c == a * a) ||
           (c * c + a * a == b * b);
}

void log_result(int a, int b, int c, int result) {
    pthread_mutex_lock(&file_mutex);
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "(%d, %d, %d) - %s\n", a, b, c, result ? "Pythagorean Triple" : "Not a Triple");
        fclose(log_file);
    }
    pthread_mutex_unlock(&file_mutex);
}

void *report_thread_func(void *arg) {
    while (running) {
        pthread_mutex_lock(&report_mutex);
        while (running && !reporting_started) {
            pthread_cond_wait(&report_cond, &report_mutex);
        }
        if (!running) {
            pthread_mutex_unlock(&report_mutex);
            break;
        }
        printf("Checked %d triangles, Found %d Pythagorean triples\n", total_checked, pythagorean_count);
        reporting_started = 0; // Reset flag
        pthread_mutex_unlock(&report_mutex);
    }
    printf("Report thread exiting...\n");
    return NULL;
}

void handle_client_request(void *arg) {
    int client_fd = (int)(intptr_t)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);  // Blocking recv
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Client disconnected.\n");
            } else {
                perror("recv failed");
            }
            close(client_fd);
            break;
        }

        buffer[bytes_received] = '\0';
        int side = atoi(buffer);

        pthread_mutex_lock(&sides_mutex);
        sides[side_count % 3] = side;
        side_count++;

        if (side_count >= 3) {
            int a = sides[0], b = sides[1], c = sides[2];
            int result = is_pythagorean_triple(a, b, c);
            log_result(a, b, c, result);

            pthread_mutex_lock(&report_mutex);
            total_checked++;
            if (result) pythagorean_count++;

            // Start reporting thread only after checking 10 triangles
            if (total_checked % 10 == 0) {
                reporting_started = 1;
                pthread_cond_signal(&report_cond);
            }
            pthread_mutex_unlock(&report_mutex);
        }
        pthread_mutex_unlock(&sides_mutex);

    }
}

// Signal handler function
void signal_handler(int signum) {
    printf("\nCaught SIGINT (Ctrl+C), shutting down server...\n");
    
    // Stop the reporting thread
    running = 0;
    
    // Wake up the report thread if it's waiting
    pthread_mutex_lock(&report_mutex);
    pthread_cond_signal(&report_cond);
    pthread_mutex_unlock(&report_mutex);

    // Close the server socket to break accept()
    close(server_fd);

    // Destroy mutexes and condition variable
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&sides_mutex);
    pthread_mutex_destroy(&report_mutex);
    pthread_cond_destroy(&report_cond);

    // Destroy the thread pool before exiting
    thread_pool_destroy();

    exit(EXIT_SUCCESS);
}

int main() {
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Register SIGINT handler
    signal(SIGINT, signal_handler);

    // Reset the result log file by opening it in write mode ("w")
    FILE *log_file = fopen(LOG_FILE, "w");
    if (log_file) {
        fclose(log_file);  // Close the file immediately to reset it
    } else {
        perror("Failed to reset log file");
        exit(EXIT_FAILURE);
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, THREAD_POOL_SIZE) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);
    
    // Initialize the thread pool
    thread_pool_init(THREAD_POOL_SIZE);

    // Start the reporting thread
    pthread_t report_thread;
    pthread_create(&report_thread, NULL, report_thread_func, NULL);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            if (errno == EINTR) {
                break;  // Exit if interrupted by SIGINT
            }
            continue;
        }

        printf("New client connected.\n");

        // Add the client handler to the thread pool
        thread_pool_add_task(handle_client_request, (void *)(intptr_t)client_fd);
    }

    printf("Server has shut down gracefully.\n");
    return 0;
}

