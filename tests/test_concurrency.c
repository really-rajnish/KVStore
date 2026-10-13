#include "kvstore.h"
#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define NUM_WORKERS 16
#define OPS_PER_WORKER 1000
#define NUM_KEYS 50

static int total_asserts = 0;
static int failed_asserts = 0;

#define TEST_ASSERT(cond, msg) do { \
    total_asserts++; \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        failed_asserts++; \
    } else { \
        printf("PASS: %s\n", msg); \
    } \
    fflush(stdout); \
} while(0)

typedef struct {
    KVStore *kv;
    int      worker_id;
} WorkerArgs;

static void *worker_func(void *arg)
{
    WorkerArgs *wargs = (WorkerArgs *)arg;
    KVStore *kv = wargs->kv;
    int id = wargs->worker_id;

    char key[32];
    char val[64];
    char out[64];

    for (int i = 0; i < OPS_PER_WORKER; i++) {
        int key_idx = (id * 17 + i) % NUM_KEYS;
        snprintf(key, sizeof(key), "key:%d", key_idx);

        int op = (i + id) % 10;
        if (op < 7) {
            /* 70% GET */
            kvstore_get_copy(kv, key, out, sizeof(out));
        } else if (op < 9) {
            /* 20% SET */
            snprintf(val, sizeof(val), "val_w%d_%d", id, i);
            kvstore_set(kv, key, val);
        } else {
            /* 10% DEL */
            kvstore_del(kv, key);
        }
    }
    return NULL;
}

static void test_concurrency_mode(LockMode mode, const char *mode_name)
{
    printf("\n--- Concurrency Test (%s mode): %d threads x %d ops = %d total ops ---\n",
           mode_name, NUM_WORKERS, OPS_PER_WORKER, NUM_WORKERS * OPS_PER_WORKER);

    KVStore *kv = kvstore_create();
    kvstore_set_lock_mode(kv, mode);

    /* Pre-populate some keys */
    for (int i = 0; i < NUM_KEYS; i++) {
        char k[32], v[32];
        snprintf(k, sizeof(k), "key:%d", i);
        snprintf(v, sizeof(v), "init_%d", i);
        kvstore_set(kv, k, v);
    }

    pthread_t threads[NUM_WORKERS];
    WorkerArgs args[NUM_WORKERS];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_WORKERS; i++) {
        args[i].kv        = kv;
        args[i].worker_id = i;
        int rc = pthread_create(&threads[i], NULL, worker_func, &args[i]);
        TEST_ASSERT(rc == 0, "spawn concurrency worker thread");
    }

    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double ops_sec = (NUM_WORKERS * OPS_PER_WORKER) / elapsed;

    printf("Info: %s mode completed in %.4f seconds (%.0f OPS)\n", mode_name, elapsed, ops_sec);

    /* Verify store is intact and functional */
    size_t final_size = kvstore_size(kv);
    TEST_ASSERT(final_size <= NUM_KEYS, "final size bounded by unique keys");
    printf("Info: Final live keys: %zu\n", final_size);

    kvstore_destroy(kv);
}

static void test_threadpool_basic(void)
{
    printf("\n--- ThreadPool Unit Test ---\n");
    ThreadPool *pool = threadpool_create(4, 32);
    TEST_ASSERT(pool != NULL, "threadpool_create() 4 workers");

    int counter = 0;
    pthread_mutex_t cnt_lock = PTHREAD_MUTEX_INITIALIZER;

    void add_task(void *a) {
        int *c = (int *)a;
        pthread_mutex_lock(&cnt_lock);
        (*c)++;
        pthread_mutex_unlock(&cnt_lock);
    }

    for (int i = 0; i < 20; i++) {
        TEST_ASSERT(threadpool_submit(pool, add_task, &counter), "submit task to pool");
    }

    threadpool_destroy(pool);
    TEST_ASSERT(counter == 20, "all 20 tasks executed by threadpool");
}

int main(void)
{
    printf("Running Concurrency & Striped Locking tests (Week 10)...\n");
    test_threadpool_basic();
    test_concurrency_mode(KV_LOCK_STRIPED, "STRIPED (64 RW-Locks)");
    test_concurrency_mode(KV_LOCK_GLOBAL, "GLOBAL (Single Mutex)");

    if (failed_asserts == 0) {
        printf("\nALL CONCURRENCY TESTS PASSED (%d assertions, 0 failures)\n", total_asserts);
        return 0;
    } else {
        printf("\nCONCURRENCY TESTS FAILED (%d failures out of %d assertions)\n", failed_asserts, total_asserts);
        return 1;
    }
}
