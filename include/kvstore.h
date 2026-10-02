#ifndef KVSTORE_H
#define KVSTORE_H

#include <stddef.h>
#include <stdbool.h>

#include "hash_table.h"

typedef struct {
    HashTable *table;
} KVStore;

KVStore *kvstore_create(void);
void     kvstore_destroy(KVStore *kv);

bool     kvstore_set(KVStore *kv, const char *key, const char *value);
char    *kvstore_get(KVStore *kv, const char *key);
bool     kvstore_del(KVStore *kv, const char *key);
size_t   kvstore_size(const KVStore *kv);

#endif /* KVSTORE_H */
