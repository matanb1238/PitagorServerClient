#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>  // Include this header for the sleep function

// Task structure and thread pool structure
typedef struct task {
    void (*function)(void *);  // Task function pointer
    void *arg;                 // Argument for the function
    struct task *next;         // Pointer to the next task
} task_t;

typedef struct {
    pthread_t *threads;        // Array of threads
    task_t *task_head;         // Pointer to the first task in the queue
    task_t *task_tail;         // Pointer to the last task in the queue
    int thread_count;          // Number of threads
    int stop;                  // Stop flag
    pthread_mutex_t lock;      // Mutex for thread synchronization
    pthread_cond_t notify;     // Condition variable for notifying threads
} thread_pool_t;

thread_pool_t pool;

// Example task function
void example_task(void *arg) {
    int *task_id = (int *)arg;
    printf("Processing task: %d\n", *task_id);
    free(task_id);  // Don't forget to free dynamically allocated memory
}

// Worker thread function
void *thread_worker(void *arg) {
    while (1) {
        pthread_mutex_lock(&pool.lock);

        while (pool.task_head == NULL && !pool.stop) {
            pthread_cond_wait(&pool.notify, &pool.lock);
        }

        if (pool.stop) {
            pthread_mutex_unlock(&pool.lock);
            break;
        }

        task_t *task = pool.task_head;
        if (task) {
            pool.task_head = task->next;
            if (pool.task_head == NULL) {
                pool.task_tail = NULL;
            }
        }

        pthread_mutex_unlock(&pool.lock);

        if (task) {
            task->function(task->arg);
            free(task);
        }
    }
    return NULL;
}

// Initialize thread pool
void thread_pool_init(int num_threads) {
    pool.task_head = NULL;
    pool.task_tail = NULL;
    pool.thread_count = num_threads;
    pool.stop = 0;

    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.notify, NULL);

    pool.threads = malloc(num_threads * sizeof(pthread_t));
    if (!pool.threads) {
        perror("Failed to allocate memory for threads");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool.threads[i], NULL, thread_worker, NULL);
    }
}

// Add task to the queue
void thread_pool_add_task(void (*function)(void *), void *arg) {
    task_t *new_task = malloc(sizeof(task_t));
    if (!new_task) {
        perror("Failed to allocate memory for new task");
        return;
    }

    new_task->function = function;
    new_task->arg = arg;
    new_task->next = NULL;

    pthread_mutex_lock(&pool.lock);

    if (pool.task_tail) {
        pool.task_tail->next = new_task;
    } else {
        pool.task_head = new_task;
    }
    pool.task_tail = new_task;

    pthread_cond_signal(&pool.notify);
    pthread_mutex_unlock(&pool.lock);
}

// Destroy thread pool
void thread_pool_destroy() {
    pthread_mutex_lock(&pool.lock);
    pool.stop = 1;
    pthread_cond_broadcast(&pool.notify);
    pthread_mutex_unlock(&pool.lock);

    for (int i = 0; i < pool.thread_count; i++) {
        pthread_join(pool.threads[i], NULL);
    }

    free(pool.threads);

    while (pool.task_head) {
        task_t *task = pool.task_head;
        pool.task_head = task->next;
        free(task);
    }

    pthread_mutex_destroy(&pool.lock);
    pthread_cond_destroy(&pool.notify);
}
