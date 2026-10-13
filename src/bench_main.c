#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#include "kvstore.h"

#define MAX_SAMPLES_PER_THREAD 10000

typedef struct {
    KVStore     *kv;
    int          thread_id;
    int          ops;
    int          key_range;
    int          read_ratio;
    uint64_t     latencies[MAX_SAMPLES_PER_THREAD];
    size_t       sample_count;
} BenchThreadArgs;

static int cmp_u64(const void *a, const void *b)
{
    uint64_t va = *(const uint64_t *)a;
    uint64_t vb = *(const uint64_t *)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static void *bench_worker(void *arg)
{
    BenchThreadArgs *args = (BenchThreadArgs *)arg;
    KVStore *kv = args->kv;
    int id = args->thread_id;
    int n = args->ops;
    int krange = args->key_range;
    int rratio = args->read_ratio;

    char key[32];
    char val[64];
    char out[64];

    /* Sample every K operations to avoid memory explosion while maintaining statistical accuracy */
    int sample_stride = n / MAX_SAMPLES_PER_THREAD;
    if (sample_stride < 1) sample_stride = 1;

    for (int i = 0; i < n; i++) {
        int key_num = (id * 997 + i * 31) % krange;
        snprintf(key, sizeof(key), "bench_k:%d", key_num);

        int dice = (i * 17 + id) % 100;
        bool do_sample = (i % sample_stride == 0) && (args->sample_count < MAX_SAMPLES_PER_THREAD);

        struct timespec t0, t1;
        if (do_sample) clock_gettime(CLOCK_MONOTONIC, &t0);

        if (dice < rratio) {
            kvstore_get_copy(kv, key, out, sizeof(out));
        } else {
            snprintf(val, sizeof(val), "val_%d_%d", id, i);
            kvstore_set(kv, key, val);
        }

        if (do_sample) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            int64_t dt_sec = t1.tv_sec - t0.tv_sec;
            int64_t dt_nsec = t1.tv_nsec - t0.tv_nsec;
            if (dt_nsec < 0) {
                dt_sec -= 1;
                dt_nsec += 1000000000L;
            }
            uint64_t lat = (uint64_t)(dt_sec * 1000000LL + dt_nsec / 1000LL);
            args->latencies[args->sample_count++] = lat;
        }
    }

    return NULL;
}

typedef struct {
    double throughput_ops;
    double p50_us;
    double p90_us;
    double p99_us;
    double max_us;
} BenchResult;

static BenchResult run_benchmark(LockMode mode, const char *label, int num_threads, int ops_per_thread, int key_range, int read_ratio)
{
    KVStore *kv = kvstore_create();
    kvstore_set_lock_mode(kv, mode);

    /* Pre-populate key space */
    for (int i = 0; i < key_range; i++) {
        char k[32], v[32];
        snprintf(k, sizeof(k), "bench_k:%d", i);
        snprintf(v, sizeof(v), "init_val_%d", i);
        kvstore_set(kv, k, v);
    }

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    BenchThreadArgs *args = malloc(num_threads * sizeof(BenchThreadArgs));

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_threads; i++) {
        args[i].kv           = kv;
        args[i].thread_id    = i;
        args[i].ops          = ops_per_thread;
        args[i].key_range    = key_range;
        args[i].read_ratio   = read_ratio;
        args[i].sample_count = 0;
        pthread_create(&threads[i], NULL, bench_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double total_ops = (double)num_threads * (double)ops_per_thread;
    double ops_sec = total_ops / elapsed;

    /* Aggregate latency samples */
    size_t total_samples = 0;
    for (int i = 0; i < num_threads; i++) total_samples += args[i].sample_count;

    uint64_t *all_lats = malloc(total_samples * sizeof(uint64_t));
    size_t offset = 0;
    for (int i = 0; i < num_threads; i++) {
        memcpy(all_lats + offset, args[i].latencies, args[i].sample_count * sizeof(uint64_t));
        offset += args[i].sample_count;
    }

    qsort(all_lats, total_samples, sizeof(uint64_t), cmp_u64);

    BenchResult res;
    res.throughput_ops = ops_sec;
    res.p50_us = total_samples ? (double)all_lats[(size_t)(total_samples * 0.50)] : 0.0;
    res.p90_us = total_samples ? (double)all_lats[(size_t)(total_samples * 0.90)] : 0.0;
    res.p99_us = total_samples ? (double)all_lats[(size_t)(total_samples * 0.99)] : 0.0;
    res.max_us = total_samples ? (double)all_lats[total_samples - 1] : 0.0;

    printf("[%-16s] Time: %6.3fs | Throughput: %10.0f OPS | p50: %4.1fus | p90: %4.1fus | p99: %4.1fus | max: %6.1fus\n",
           label, elapsed, res.throughput_ops, res.p50_us, res.p90_us, res.p99_us, res.max_us);

    free(all_lats);
    free(threads);
    free(args);
    kvstore_destroy(kv);
    return res;
}

int main(int argc, char **argv)
{
    int num_threads    = 8;
    int ops_per_thread = 25000;
    int key_range      = 2000;
    int read_ratio     = 80;
    const char *mode   = "both";

    int opt;
    while ((opt = getopt(argc, argv, "t:n:k:r:m:h")) != -1) {
        switch (opt) {
        case 't': num_threads = atoi(optarg); break;
        case 'n': ops_per_thread = atoi(optarg); break;
        case 'k': key_range = atoi(optarg); break;
        case 'r': read_ratio = atoi(optarg); break;
        case 'm': mode = optarg; break;
        case 'h':
        default:
            printf("Usage: %s [-t threads] [-n ops_per_thread] [-k keys] [-r read_pct] [-m mode (striped|global|both)]\n", argv[0]);
            return 0;
        }
    }

    printf("================================================================================\n");
    printf(" KVStore High-Concurrency Benchmark Suite (Week 12 Empirical Evaluation)\n");
    printf(" Config: %d threads, %d ops/thread (%d total ops), %d keys, %d%% reads / %d%% writes\n",
           num_threads, ops_per_thread, num_threads * ops_per_thread, key_range, read_ratio, 100 - read_ratio);
    printf("================================================================================\n");

    if (strcmp(mode, "global") == 0) {
        run_benchmark(KV_LOCK_GLOBAL, "Global Mutex", num_threads, ops_per_thread, key_range, read_ratio);
    } else if (strcmp(mode, "striped") == 0) {
        run_benchmark(KV_LOCK_STRIPED, "32-Stripe RW-Lock", num_threads, ops_per_thread, key_range, read_ratio);
    } else {
        BenchResult g_res = run_benchmark(KV_LOCK_GLOBAL, "Global Mutex", num_threads, ops_per_thread, key_range, read_ratio);
        BenchResult s_res = run_benchmark(KV_LOCK_STRIPED, "32-Stripe RW-Lock", num_threads, ops_per_thread, key_range, read_ratio);

        double speedup = s_res.throughput_ops / (g_res.throughput_ops > 0 ? g_res.throughput_ops : 1.0);
        printf("--------------------------------------------------------------------------------\n");
        printf(" Speedup Factor (Striped RW vs Global Mutex): %.2fx\n", speedup);
        printf(" Latency Reduction (p99): %.1fus -> %.1fus (%.1f%% improvement)\n",
               g_res.p99_us, s_res.p99_us,
               g_res.p99_us > 0 ? ((g_res.p99_us - s_res.p99_us) / g_res.p99_us * 100.0) : 0.0);
        printf("================================================================================\n");
    }

    return 0;
}
