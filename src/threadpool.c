#include "threadpool.h"

#include <stdlib.h>
#include <pthread.h>

typedef struct TaskNode {
    ThreadPoolTaskFn  fn;
    void             *arg;
    struct TaskNode  *next;
} TaskNode;

struct ThreadPool {
    pthread_t       *threads;
    size_t           num_threads;
    size_t           queue_capacity;
    size_t           queue_size;

    TaskNode        *queue_head;
    TaskNode        *queue_tail;

    pthread_mutex_t  lock;
    pthread_cond_t   not_empty;
    pthread_cond_t   not_full;
    bool             shutdown;
};

static void *worker_routine(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;

    while (1) {
        pthread_mutex_lock(&pool->lock);

        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }

        if (pool->shutdown && pool->queue_size == 0) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        TaskNode *task = pool->queue_head;
        pool->queue_head = task->next;
        if (!pool->queue_head) {
            pool->queue_tail = NULL;
        }
        pool->queue_size--;

        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->lock);

        /* Execute task outside lock */
        task->fn(task->arg);
        free(task);
    }
    return NULL;
}

ThreadPool *threadpool_create(size_t num_threads, size_t queue_capacity)
{
    if (num_threads == 0 || queue_capacity == 0) return NULL;

    ThreadPool *pool = malloc(sizeof *pool);
    if (!pool) return NULL;

    pool->num_threads    = num_threads;
    pool->queue_capacity = queue_capacity;
    pool->queue_size     = 0;
    pool->queue_head     = NULL;
    pool->queue_tail     = NULL;
    pool->shutdown       = false;

    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (!pool->threads) {
        free(pool);
        return NULL;
    }

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);

    for (size_t i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_routine, pool) != 0) {
            pool->shutdown = true;
            pthread_cond_broadcast(&pool->not_empty);
            for (size_t j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            free(pool->threads);
            pthread_mutex_destroy(&pool->lock);
            pthread_cond_destroy(&pool->not_empty);
            pthread_cond_destroy(&pool->not_full);
            free(pool);
            return NULL;
        }
    }

    return pool;
}

bool threadpool_submit(ThreadPool *pool, ThreadPoolTaskFn fn, void *arg)
{
    if (!pool || !fn) return false;

    pthread_mutex_lock(&pool->lock);

    while (pool->queue_size >= pool->queue_capacity && !pool->shutdown) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return false;
    }

    TaskNode *node = malloc(sizeof *node);
    if (!node) {
        pthread_mutex_unlock(&pool->lock);
        return false;
    }
    node->fn   = fn;
    node->arg  = arg;
    node->next = NULL;

    if (pool->queue_tail) {
        pool->queue_tail->next = node;
        pool->queue_tail = node;
    } else {
        pool->queue_head = node;
        pool->queue_tail = node;
    }
    pool->queue_size++;

    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
    return true;
}

void threadpool_destroy(ThreadPool *pool)
{
    if (!pool) return;

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->not_empty);
    pthread_cond_broadcast(&pool->not_full);
    pthread_mutex_unlock(&pool->lock);

    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    TaskNode *curr = pool->queue_head;
    while (curr) {
        TaskNode *next = curr->next;
        free(curr);
        curr = next;
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
    free(pool->threads);
    free(pool);
}
