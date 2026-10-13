#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

typedef struct HTEntry {
    char            *key;
    char            *value;
    int64_t          expire_at_ms; /* 0 means no expiry */
    struct HTEntry  *next;
} HTEntry;

typedef struct {
    HTEntry        *buckets;
    size_t          capacity;
    atomic_size_t   size;
} HashTable;

HashTable    *ht_create(size_t initial_capacity);
void          ht_destroy(HashTable *ht);

bool          ht_set(HashTable *ht, const char *key, const char *value);
bool          ht_set_ttl(HashTable *ht, const char *key, const char *value, int64_t expire_at_ms);
char         *ht_get(HashTable *ht, const char *key);
HTEntry      *ht_find_entry(const HashTable *ht, const char *key);
bool          ht_del(HashTable *ht, const char *key);
size_t        ht_size(const HashTable *ht);
bool          ht_resize_table(HashTable *ht, size_t new_capacity);

unsigned long hash_djb2(const char *s);

#endif /* HASH_TABLE_H */
