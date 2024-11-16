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
#define REPORT_INTERVAL 10

// Mutex and condition variable
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Job structure
typedef struct Job {
    int client_fd;
    struct Job* next;
} Job;

// Global job queue
Job* job_queue = NULL;

// Pythagorean triple counter
int pythagorean_count = 0;

// Function to check if a triple is Pythagorean
int is_pythagorean_triple(unsigned int a, unsigned int b, unsigned int c) {
    return (a * a + b * b == c * c) || (a * a + c * c == b * b) || (b * b + c * c == a * a);
}

// Function to handle client requests
void handle_client_request(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        unsigned int a, b, c;
        if (sscanf(buffer, "%u %u %u", &a, &b, &c) == 3) {
            int result = is_pythagorean_triple(a, b, c);

            // Update Pythagorean triple counter
            pthread_mutex_lock(&queue_mutex);
            if (result) {
                pythagorean_count++;
            }
            pthread_mutex_unlock(&queue_mutex);

            // Send response to client
            char response[100];
            snprintf(response, sizeof(response), "Sides: %u %u %u - %s\n", 
                     a, b, c, result ? "Pythagorean" : "Not Pythagorean");
            send(client_fd, response, strlen(response), 0);
        }
    }
    close(client_fd);
}

// Worker thread function
void* worker_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&queue_mutex);

        // Wait for a job to be available
        while (job_queue == NULL) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }

        // Dequeue a job
        Job* job = job_queue;
        job_queue = job_queue->next;
        pthread_mutex_unlock(&queue_mutex);

        // Process the job
        handle_client_request(job->client_fd);
        free(job);
    }
    return NULL;
}

// Supervisor thread function
void* supervisor_thread(void* arg) {
    while (1) {
        sleep(1); // Periodic check (adjust time as needed)

        pthread_mutex_lock(&queue_mutex);
        if (pythagorean_count > 0 && pythagorean_count % REPORT_INTERVAL == 0) {
            printf("Supervisor: Pythagorean triples found so far: %d\n", pythagorean_count);
        }
        pthread_mutex_unlock(&queue_mutex);
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

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Create thread pool
    pthread_t thread_pool[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&thread_pool[i], NULL, worker_thread, NULL) != 0) {
            perror("pthread_create");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    // Create supervisor thread
    pthread_t supervisor;
    if (pthread_create(&supervisor, NULL, supervisor_thread, NULL) != 0) {
        perror("pthread_create");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Accept and enqueue client requests
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        printf("New client connected: %d\n", client_fd);
        enqueue_job(client_fd);
    }

    // Cleanup (never reached in this example)
    close(server_fd);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(thread_pool[i], NULL);
    }
    pthread_join(supervisor, NULL);

    return 0;
}
