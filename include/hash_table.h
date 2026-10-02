#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>
#include <stdbool.h>

/* One entry in a collision chain. */
typedef struct HTEntry {
    char            *key;
    char            *value;
    struct HTEntry  *next;
} HTEntry;

/* Chained hash table with sentinel head nodes. */
typedef struct {
    HTEntry *buckets;   /* array of sentinel heads, length = capacity */
    size_t   capacity;  /* number of buckets                              */
    size_t   size;      /* number of live keys                            */
} HashTable;

HashTable *ht_create(size_t initial_capacity);
void       ht_destroy(HashTable *ht);

/* Copies both key and value. Returns false on allocation failure. */
bool       ht_set(HashTable *ht, const char *key, const char *value);

/* Returns a pointer to the internal value, or NULL. Do NOT free it. */
char      *ht_get(HashTable *ht, const char *key);

/* Returns true if the key existed and was removed. */
bool       ht_del(HashTable *ht, const char *key);

size_t     ht_size(const HashTable *ht);

#endif /* HASH_TABLE_H */
