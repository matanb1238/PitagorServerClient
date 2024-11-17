#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define QUEUE_SIZE 10
#define THREAD_POOL_SIZE 3

// Task structure
typedef struct {
    void (*function)(void *);  // Task function
    void *arg;                // Task argument
} task_t;

// Thread pool structure
typedef struct {
    pthread_t threads[THREAD_POOL_SIZE]; // Worker threads
    task_t task_queue[QUEUE_SIZE];       // Task queue (circular buffer)
    int queue_front;                    // Front index of the queue
    int queue_rear;                     // Rear index of the queue
    int count;                          // Number of tasks in the queue
    pthread_mutex_t lock;               // Mutex for queue synchronization
    pthread_cond_t notify;              // Condition variable to signal threads
} thread_pool_t;

// Global thread pool
thread_pool_t pool;

// Function to process client requests
void process_client_request(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[1024];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Client %d sent: %s\n", client_fd, buffer);
    } else {
        perror("recv");
    }

    close(client_fd);  // Close the connection
    free(arg);         // Free dynamically allocated memory
}

// Initialize the thread pool
void thread_pool_init(int num_threads) {
    pool.queue_front = 0;
    pool.queue_rear = 0;
    pool.count = 0;
    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.notify, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool.threads[i], NULL, thread_worker, NULL);
    }
}

// Add a task to the thread pool
void thread_pool_add_task(void (*function)(void *), void *arg) {
    pthread_mutex_lock(&pool.lock);

    // Add task to the queue
    pool.task_queue[pool.queue_rear].function = function;
    pool.task_queue[pool.queue_rear].arg = arg;
    pool.queue_rear = (pool.queue_rear + 1) % QUEUE_SIZE;
    pool.count++;

    // Signal a waiting thread
    pthread_cond_signal(&pool.notify);
    pthread_mutex_unlock(&pool.lock);
}

// Worker thread function
void *thread_worker(void *arg) {
    while (1) {
        pthread_mutex_lock(&pool.lock);

        // Wait for a task
        while (pool.count == 0) {
            pthread_cond_wait(&pool.notify, &pool.lock);
        }

        // Get the next task from the queue
        task_t task = pool.task_queue[pool.queue_front];
        pool.queue_front = (pool.queue_front + 1) % QUEUE_SIZE;
        pool.count--;

        pthread_mutex_unlock(&pool.lock);

        // Execute the task
        task.function(task.arg);
    }
    return NULL;
}

// Main server function
int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind and listen
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

    printf("Server listening on port %d...\n", PORT);

    // Initialize thread pool
    thread_pool_init(THREAD_POOL_SIZE);

    // Accept client connections
    while (1) {
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (*client_fd == -1) {
            perror("accept");
            free(client_fd);
            continue;
        }

        printf("New client connected: %d\n", *client_fd);

        // Add the client request to the thread pool
        thread_pool_add_task(process_client_request, client_fd);
    }

    close(server_fd);
    return 0;
}
