#include "kvstore.h"
#include "wal.h"
#include "ttl.h"
#include "compaction.h"

#include <stdlib.h>
#include <string.h>

#define MAX_LOAD_FACTOR 0.75

static size_t get_key_stripe(const KVStore *kv, const char *key)
{
    size_t bucket = (size_t)(hash_djb2(key) % kv->table->capacity);
    return bucket % STRIPE_COUNT;
}

static void lock_key_read(KVStore *kv, const char *key)
{
    if (kv->lock_mode == KV_LOCK_GLOBAL) {
        pthread_mutex_lock(&kv->global_lock);
    } else {
        size_t idx = get_key_stripe(kv, key);
        pthread_rwlock_rdlock(&kv->stripes[idx]);
    }
}

static void unlock_key_read(KVStore *kv, const char *key)
{
    if (kv->lock_mode == KV_LOCK_GLOBAL) {
        pthread_mutex_unlock(&kv->global_lock);
    } else {
        size_t idx = get_key_stripe(kv, key);
        pthread_rwlock_unlock(&kv->stripes[idx]);
    }
}

static void lock_key_write(KVStore *kv, const char *key)
{
    if (kv->lock_mode == KV_LOCK_GLOBAL) {
        pthread_mutex_lock(&kv->global_lock);
    } else {
        size_t idx = get_key_stripe(kv, key);
        pthread_rwlock_wrlock(&kv->stripes[idx]);
    }
}

static void unlock_key_write(KVStore *kv, const char *key)
{
    if (kv->lock_mode == KV_LOCK_GLOBAL) {
        pthread_mutex_unlock(&kv->global_lock);
    } else {
        size_t idx = get_key_stripe(kv, key);
        pthread_rwlock_unlock(&kv->stripes[idx]);
    }
}

static void lock_all_write(KVStore *kv)
{
    if (kv->lock_mode == KV_LOCK_GLOBAL) {
        pthread_mutex_lock(&kv->global_lock);
    } else {
        for (size_t i = 0; i < STRIPE_COUNT; i++) {
            pthread_rwlock_wrlock(&kv->stripes[i]);
        }
    }
}

static void unlock_all_write(KVStore *kv)
{
    if (kv->lock_mode == KV_LOCK_GLOBAL) {
        pthread_mutex_unlock(&kv->global_lock);
    } else {
        for (size_t i = 0; i < STRIPE_COUNT; i++) {
            pthread_rwlock_unlock(&kv->stripes[i]);
        }
    }
}

KVStore *kvstore_create(void)
{
    return kvstore_create_with_wal(NULL, WAL_SYNC_NEVER);
}

KVStore *kvstore_create_with_wal(const char *wal_path, WalSyncPolicy sync_policy)
{
    KVStore *kv = malloc(sizeof *kv);
    if (!kv) return NULL;

    kv->table = ht_create(16);
    if (!kv->table) {
        free(kv);
        return NULL;
    }
    kv->wal         = NULL;
    kv->wal_path    = wal_path ? strdup(wal_path) : NULL;
    kv->sync_policy = sync_policy;
    kv->lock_mode   = KV_LOCK_STRIPED;
    kv->reaper      = NULL;

    pthread_mutex_init(&kv->global_lock, NULL);
    for (size_t i = 0; i < STRIPE_COUNT; i++) {
        pthread_rwlock_init(&kv->stripes[i], NULL);
    }

    if (wal_path) {
        wal_replay(kv, wal_path);
        kv->wal = wal_open(wal_path, sync_policy);
    }

    return kv;
}

void kvstore_destroy(KVStore *kv)
{
    if (!kv) return;

    if (kv->reaper) {
        ttl_reaper_stop(kv->reaper);
        kv->reaper = NULL;
    }

    if (kv->wal) {
        wal_close(kv->wal);
        kv->wal = NULL;
    }
    free(kv->wal_path);
    ht_destroy(kv->table);

    pthread_mutex_destroy(&kv->global_lock);
    for (size_t i = 0; i < STRIPE_COUNT; i++) {
        pthread_rwlock_destroy(&kv->stripes[i]);
    }
    free(kv);
}

void kvstore_set_lock_mode(KVStore *kv, LockMode mode)
{
    if (!kv) return;
    LockMode old_mode = kv->lock_mode;
    if (old_mode == KV_LOCK_GLOBAL) {
        pthread_mutex_lock(&kv->global_lock);
    } else {
        for (size_t i = 0; i < STRIPE_COUNT; i++) {
            pthread_rwlock_wrlock(&kv->stripes[i]);
        }
    }

    kv->lock_mode = mode;

    if (old_mode == KV_LOCK_GLOBAL) {
        pthread_mutex_unlock(&kv->global_lock);
    } else {
        for (size_t i = 0; i < STRIPE_COUNT; i++) {
            pthread_rwlock_unlock(&kv->stripes[i]);
        }
    }
}

LockMode kvstore_get_lock_mode(const KVStore *kv)
{
    return kv ? kv->lock_mode : KV_LOCK_STRIPED;
}

bool kvstore_set_ttl(KVStore *kv, const char *key, const char *value, int64_t ttl_ms)
{
    if (!kv || !key || !value) return false;

    if ((double)(atomic_load(&kv->table->size) + 1) / (double)kv->table->capacity > MAX_LOAD_FACTOR) {
        lock_all_write(kv);
        if ((double)(atomic_load(&kv->table->size) + 1) / (double)kv->table->capacity > MAX_LOAD_FACTOR) {
            ht_resize_table(kv->table, kv->table->capacity * 2);
        }
        unlock_all_write(kv);
    }

    int64_t expire_at_ms = (ttl_ms > 0) ? (ttl_current_time_ms() + ttl_ms) : 0;

    lock_key_write(kv, key);

    if (kv->wal) {
        bool wal_ok = false;
        if (expire_at_ms > 0) {
            wal_ok = wal_append_set_ttl(kv->wal, key, value, expire_at_ms);
        } else {
            wal_ok = wal_append_set(kv->wal, key, value);
        }
        if (!wal_ok) {
            unlock_key_write(kv, key);
            return false;
        }
    }

    bool ok = ht_set_ttl(kv->table, key, value, expire_at_ms);
    unlock_key_write(kv, key);
    return ok;
}

bool kvstore_set(KVStore *kv, const char *key, const char *value)
{
    return kvstore_set_ttl(kv, key, value, 0);
}

char *kvstore_get(KVStore *kv, const char *key)
{
    if (!kv || !key) return NULL;

    lock_key_read(kv, key);
    HTEntry *e = ht_find_entry(kv->table, key);
    if (!e) {
        unlock_key_read(kv, key);
        return NULL;
    }

    int64_t now_ms = ttl_current_time_ms();
    if (e->expire_at_ms > 0 && now_ms >= e->expire_at_ms) {
        unlock_key_read(kv, key);

        /* Lazy eviction under write lock */
        lock_key_write(kv, key);
        HTEntry *we = ht_find_entry(kv->table, key);
        if (we && we->expire_at_ms > 0 && ttl_current_time_ms() >= we->expire_at_ms) {
            if (kv->wal) wal_append_del(kv->wal, key);
            ht_del(kv->table, key);
        }
        unlock_key_write(kv, key);
        return NULL;
    }

    char *v = e->value;
    unlock_key_read(kv, key);
    return v;
}

bool kvstore_get_copy(KVStore *kv, const char *key, char *out_buf, size_t max_len)
{
    if (!kv || !key || !out_buf || max_len == 0) return false;

    lock_key_read(kv, key);
    HTEntry *e = ht_find_entry(kv->table, key);
    if (!e) {
        unlock_key_read(kv, key);
        return false;
    }

    int64_t now_ms = ttl_current_time_ms();
    if (e->expire_at_ms > 0 && now_ms >= e->expire_at_ms) {
        unlock_key_read(kv, key);

        /* Lazy eviction under write lock */
        lock_key_write(kv, key);
        HTEntry *we = ht_find_entry(kv->table, key);
        if (we && we->expire_at_ms > 0 && ttl_current_time_ms() >= we->expire_at_ms) {
            if (kv->wal) wal_append_del(kv->wal, key);
            ht_del(kv->table, key);
        }
        unlock_key_write(kv, key);
        return false;
    }

    strncpy(out_buf, e->value, max_len - 1);
    out_buf[max_len - 1] = '\0';
    unlock_key_read(kv, key);
    return true;
}

bool kvstore_del(KVStore *kv, const char *key)
{
    if (!kv || !key) return false;

    lock_key_write(kv, key);

    if (!ht_find_entry(kv->table, key)) {
        unlock_key_write(kv, key);
        return false;
    }

    if (kv->wal) {
        wal_append_del(kv->wal, key);
    }
    bool ok = ht_del(kv->table, key);
    unlock_key_write(kv, key);
    return ok;
}

size_t kvstore_size(const KVStore *kv)
{
    if (!kv) return 0;
    return ht_size(kv->table);
}

bool kvstore_compact(KVStore *kv)
{
    if (!kv) return false;
    if (!kv->wal || !kv->wal_path) return true;

    lock_all_write(kv);
    bool ok = compaction_run(kv);
    unlock_all_write(kv);
    return ok;
}

bool kvstore_expire(KVStore *kv, const char *key, int64_t ttl_ms)
{
    if (!kv || !key || ttl_ms <= 0) return false;

    lock_key_write(kv, key);
    HTEntry *e = ht_find_entry(kv->table, key);
    if (!e) {
        unlock_key_write(kv, key);
        return false;
    }

    int64_t now_ms = ttl_current_time_ms();
    if (e->expire_at_ms > 0 && now_ms >= e->expire_at_ms) {
        if (kv->wal) wal_append_del(kv->wal, key);
        ht_del(kv->table, key);
        unlock_key_write(kv, key);
        return false;
    }

    int64_t new_exp = now_ms + ttl_ms;
    e->expire_at_ms = new_exp;

    if (kv->wal) {
        wal_append_set_ttl(kv->wal, key, e->value, new_exp);
    }

    unlock_key_write(kv, key);
    return true;
}

int64_t kvstore_ttl(KVStore *kv, const char *key)
{
    if (!kv || !key) return -2;

    lock_key_read(kv, key);
    HTEntry *e = ht_find_entry(kv->table, key);
    if (!e) {
        unlock_key_read(kv, key);
        return -2;
    }

    if (e->expire_at_ms == 0) {
        unlock_key_read(kv, key);
        return -1;
    }

    int64_t now_ms = ttl_current_time_ms();
    if (now_ms >= e->expire_at_ms) {
        unlock_key_read(kv, key);

        /* Lazy eviction */
        lock_key_write(kv, key);
        HTEntry *we = ht_find_entry(kv->table, key);
        if (we && we->expire_at_ms > 0 && ttl_current_time_ms() >= we->expire_at_ms) {
            if (kv->wal) wal_append_del(kv->wal, key);
            ht_del(kv->table, key);
        }
        unlock_key_write(kv, key);
        return -2;
    }

    int64_t remaining = e->expire_at_ms - now_ms;
    unlock_key_read(kv, key);
    return remaining > 0 ? remaining : 0;
}

bool kvstore_start_ttl_reaper(KVStore *kv, int interval_ms)
{
    if (!kv) return false;
    if (kv->reaper) return true;
    kv->reaper = ttl_reaper_start(kv, interval_ms);
    return kv->reaper != NULL;
}

void kvstore_stop_ttl_reaper(KVStore *kv)
{
    if (!kv || !kv->reaper) return;
    ttl_reaper_stop(kv->reaper);
    kv->reaper = NULL;
}
