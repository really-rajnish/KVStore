#include "kvstore.h"

#include <stdlib.h>

KVStore *kvstore_create(void)
{
    KVStore *kv = malloc(sizeof *kv);
    if (!kv) return NULL;

    kv->table = ht_create(16);
    if (!kv->table) {
        free(kv);
        return NULL;
    }
    return kv;
}

void kvstore_destroy(KVStore *kv)
{
    if (!kv) return;
    ht_destroy(kv->table);
    free(kv);
}

bool kvstore_set(KVStore *kv, const char *key, const char *value)
{
    if (!kv) return false;
    return ht_set(kv->table, key, value);
}

char *kvstore_get(KVStore *kv, const char *key)
{
    if (!kv) return NULL;
    return ht_get(kv->table, key);
}

bool kvstore_del(KVStore *kv, const char *key)
{
    if (!kv) return false;
    return ht_del(kv->table, key);
}

size_t kvstore_size(const KVStore *kv)
{
    return kv ? ht_size(kv->table) : 0;
}
