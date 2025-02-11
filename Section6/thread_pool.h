#ifndef THREADPOOL_H
#define THREADPOOL_H

void thread_pool_init(int num_threads);
void thread_pool_add_task(void (*function)(void *), void *arg);
void thread_pool_destroy();

#endif
