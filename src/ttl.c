#include "ttl.h"
#include "kvstore.h"
#include "wal.h"
#include "hash_table.h"

#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

struct TTLReaper {
    KVStore         *kv;
    int              interval_ms;
    volatile bool    running;
    pthread_t        thread;
    pthread_mutex_t  lock;
    pthread_cond_t   cond;
};

int64_t ttl_current_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

size_t ttl_evict_expired(KVStore *kv)
{
    if (!kv || !kv->table) return 0;

    int64_t now_ms = ttl_current_time_ms();
    size_t evicted = 0;
    HashTable *ht = kv->table;

    for (size_t i = 0; i < ht->capacity; i++) {
        size_t stripe_idx = i % STRIPE_COUNT;
        if (kv->lock_mode == KV_LOCK_GLOBAL) {
            pthread_mutex_lock(&kv->global_lock);
        } else {
            pthread_rwlock_wrlock(&kv->stripes[stripe_idx]);
        }

        HTEntry *prev = &ht->buckets[i];
        HTEntry *e = prev->next;
        while (e != NULL) {
            if (e->expire_at_ms > 0 && now_ms >= e->expire_at_ms) {
                HTEntry *dead = e;
                prev->next = e->next;
                e = e->next;

                if (kv->wal) {
                    wal_append_del(kv->wal, dead->key);
                }
                free(dead->key);
                free(dead->value);
                free(dead);
                atomic_fetch_sub(&ht->size, 1);
                evicted++;
            } else {
                prev = e;
                e = e->next;
            }
        }

        if (kv->lock_mode == KV_LOCK_GLOBAL) {
            pthread_mutex_unlock(&kv->global_lock);
        } else {
            pthread_rwlock_unlock(&kv->stripes[stripe_idx]);
        }
    }

    return evicted;
}

static void *reaper_worker(void *arg)
{
    TTLReaper *r = (TTLReaper *)arg;

    pthread_mutex_lock(&r->lock);
    while (r->running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        long nsec = ts.tv_nsec + (long)(r->interval_ms % 1000) * 1000000L;
        time_t sec = ts.tv_sec + (r->interval_ms / 1000) + (nsec / 1000000000L);
        ts.tv_sec = sec;
        ts.tv_nsec = nsec % 1000000000L;

        pthread_cond_timedwait(&r->cond, &r->lock, &ts);
        if (!r->running) break;

        pthread_mutex_unlock(&r->lock);
        ttl_evict_expired(r->kv);
        pthread_mutex_lock(&r->lock);
    }
    pthread_mutex_unlock(&r->lock);
    return NULL;
}

TTLReaper *ttl_reaper_start(KVStore *kv, int interval_ms)
{
    if (!kv || interval_ms <= 0) return NULL;

    TTLReaper *r = malloc(sizeof *r);
    if (!r) return NULL;

    r->kv          = kv;
    r->interval_ms = interval_ms;
    r->running     = true;

    pthread_mutex_init(&r->lock, NULL);
    pthread_cond_init(&r->cond, NULL);

    if (pthread_create(&r->thread, NULL, reaper_worker, r) != 0) {
        pthread_mutex_destroy(&r->lock);
        pthread_cond_destroy(&r->cond);
        free(r);
        return NULL;
    }
    return r;
}

void ttl_reaper_stop(TTLReaper *r)
{
    if (!r) return;

    pthread_mutex_lock(&r->lock);
    r->running = false;
    pthread_cond_broadcast(&r->cond);
    pthread_mutex_unlock(&r->lock);

    pthread_join(r->thread, NULL);
    pthread_mutex_destroy(&r->lock);
    pthread_cond_destroy(&r->cond);
    free(r);
}
