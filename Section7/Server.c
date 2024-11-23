#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define THREAD_POOL_SIZE 3

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for the shared counter
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

typedef struct Job {
    int client_fd;
    struct Job* next;
} Job;

Job* job_queue = NULL;
unsigned int sides[3] = {0, 0, 0};  // To store the last three sides
unsigned int total_pythagorean = 0;
unsigned int valid_triangle_count = 0;  // Counter for valid Pythagorean triangles
unsigned int count = 0;  // Global counter to keep track of the received sides

// Function to check if a triple is Pythagorean
int is_pythagorean_triple(unsigned int a, unsigned int b, unsigned int c) {
    return (a * a + b * b == c * c) || (a * a + c * c == b * b) || (b * b + c * c == a * a);
}

// Function to handle client requests
void handle_client_request(int client_fd) {
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        // Handle the case when no bytes are received (client disconnect)
        if (bytes_received == 0) {
            close(client_fd);
            break;
        }
        
        // Handle socket read errors
        if (bytes_received < 0) {
            if (errno == ECONNRESET) {
                // Client disconnected
            } else {
                perror("recv");
            }
            close(client_fd);
            break;
        }

        buffer[bytes_received] = '\0';  // Null-terminate the received data
        unsigned int side;
        if (sscanf(buffer, "%u", &side) == 1) {
            // Check if the side is within the expected range (1 to 30)
            if (side < 1 || side > 30) {
                continue;  // Skip processing this side
            }

            // Lock the mutex before updating the count
            pthread_mutex_lock(&counter_mutex);

            sides[count % 3] = side;  // Circular buffer for the last three sides
            count++;
            printf("Server: Received side %u\n", side);

            if (count >= 3) {
                unsigned int a = sides[(count - 3) % 3];
                unsigned int b = sides[(count - 2) % 3];
                unsigned int c = sides[(count - 1) % 3];
                int result = is_pythagorean_triple(a, b, c);

                valid_triangle_count++;  // Increment for every valid triangle found
                if (result) {
                    total_pythagorean++;
                }

                // Check supervisor condition
                if (valid_triangle_count % 10 == 0) {
                    printf("SUPERVISOR: Checking Pythagorean triangles count: %u\n", total_pythagorean);
                }
            }

            // Unlock the mutex after modifying the counter
            pthread_mutex_unlock(&counter_mutex);
        } else {
            char error_message[] = "Invalid input. Send a positive integer.\n";
            send(client_fd, error_message, strlen(error_message), 0);
        }
    }
}

// Worker thread function
void* worker_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&queue_mutex);

        // Wait for the queue to have jobs
        while (job_queue == NULL) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }

        // Get the job from the queue
        Job* job = job_queue;
        job_queue = job_queue->next; // Move to the next job

        pthread_mutex_unlock(&queue_mutex);

        if (job != NULL) {
            handle_client_request(job->client_fd); // Handle the job
            free(job); // Free job memory after processing
        }
    }
    return NULL;
}

// Function to enqueue a job
void enqueue_job(int client_fd) {
    Job* new_job = malloc(sizeof(Job));
    if (!new_job) {
        perror("malloc");
        close(client_fd);
        return;
    }
    new_job->client_fd = client_fd;
    new_job->next = NULL;

    pthread_mutex_lock(&queue_mutex);
    if (job_queue == NULL) {
        job_queue = new_job;
    } else {
        Job* temp = job_queue;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_job;
    }
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

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

    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    pthread_t thread_pool[THREAD_POOL_SIZE];

    // Create the worker threads
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&thread_pool[i], NULL, worker_thread, NULL) != 0) {
            perror("pthread_create worker");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        enqueue_job(client_fd);
    }

    close(server_fd);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(thread_pool[i], NULL);
    }

    return 0;
}
