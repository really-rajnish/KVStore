#ifndef KVSTORE_H
#define KVSTORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "hash_table.h"

typedef enum {
    WAL_SYNC_NEVER,
    WAL_SYNC_ALWAYS
} WalSyncPolicy;

typedef enum {
    KV_LOCK_GLOBAL,
    KV_LOCK_STRIPED
} LockMode;

#define STRIPE_COUNT 32

typedef struct Wal Wal;
typedef struct TTLReaper TTLReaper;

typedef struct KVStore {
    HashTable        *table;
    Wal              *wal;
    char             *wal_path;
    WalSyncPolicy     sync_policy;
    LockMode          lock_mode;
    pthread_mutex_t   global_lock;
    pthread_rwlock_t  stripes[STRIPE_COUNT];
    TTLReaper        *reaper;
} KVStore;

KVStore *kvstore_create(void);
KVStore *kvstore_create_with_wal(const char *wal_path, WalSyncPolicy sync_policy);
void     kvstore_destroy(KVStore *kv);

void     kvstore_set_lock_mode(KVStore *kv, LockMode mode);
LockMode kvstore_get_lock_mode(const KVStore *kv);

bool     kvstore_set(KVStore *kv, const char *key, const char *value);
bool     kvstore_set_ttl(KVStore *kv, const char *key, const char *value, int64_t ttl_ms);
char    *kvstore_get(KVStore *kv, const char *key);
bool     kvstore_get_copy(KVStore *kv, const char *key, char *out_buf, size_t max_len);
bool     kvstore_del(KVStore *kv, const char *key);
size_t   kvstore_size(const KVStore *kv);
bool     kvstore_compact(KVStore *kv);

/* TTL & Key Expiry Operations */
bool     kvstore_expire(KVStore *kv, const char *key, int64_t ttl_ms);
int64_t  kvstore_ttl(KVStore *kv, const char *key);
bool     kvstore_start_ttl_reaper(KVStore *kv, int interval_ms);
void     kvstore_stop_ttl_reaper(KVStore *kv);

#endif /* KVSTORE_H */
