// thread_pool.h
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

void thread_pool_init(int num_threads);
void thread_pool_add_task(void (*function)(void *), void *arg);
void thread_pool_destroy();

#endif  // THREAD_POOL_H
