#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include <stdbool.h>

typedef void (*ThreadPoolTaskFn)(void *arg);

typedef struct ThreadPool ThreadPool;

/* Creates a worker thread pool with num_threads and bounded queue_capacity. */
ThreadPool *threadpool_create(size_t num_threads, size_t queue_capacity);

/* Submits a task to the pool. Blocks if queue is full until space is available.
   Returns true on success, false if pool is shutting down. */
bool threadpool_submit(ThreadPool *pool, ThreadPoolTaskFn fn, void *arg);

/* Gracefully stops the pool, waiting for queued tasks to finish before freeing. */
void threadpool_destroy(ThreadPool *pool);

#endif /* THREADPOOL_H */
