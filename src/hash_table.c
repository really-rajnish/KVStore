#include "hash_table.h"

#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define MIN_CAPACITY     8

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char  *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

unsigned long hash_djb2(const char *s)
{
    unsigned long h = 5381UL;
    int c;
    while ((c = (unsigned char)*s++) != 0)
        h = ((h << 5) + h) + (unsigned long)c;
    return h;
}

HTEntry *ht_find_entry(const HashTable *ht, const char *key)
{
    if (!ht || !key || ht->capacity == 0) return NULL;
    size_t idx = (size_t)(hash_djb2(key) % ht->capacity);
    for (HTEntry *e = ht->buckets[idx].next; e != NULL; e = e->next)
        if (strcmp(e->key, key) == 0)
            return e;
    return NULL;
}

bool ht_resize_table(HashTable *ht, size_t new_capacity)
{
    if (!ht || new_capacity <= ht->capacity) return false;

    HTEntry *nb = calloc(new_capacity, sizeof *nb);
    if (!nb) return false;

    for (size_t i = 0; i < ht->capacity; i++) {
        HTEntry *e = ht->buckets[i].next;
        while (e) {
            HTEntry *next = e->next;
            size_t idx = (size_t)(hash_djb2(e->key) % new_capacity);
            e->next = nb[idx].next;
            nb[idx].next = e;
            e = next;
        }
    }

    free(ht->buckets);
    ht->buckets  = nb;
    ht->capacity = new_capacity;
    return true;
}

HashTable *ht_create(size_t initial_capacity)
{
    if (initial_capacity < MIN_CAPACITY)
        initial_capacity = MIN_CAPACITY;

    HashTable *ht = malloc(sizeof *ht);
    if (!ht) return NULL;

    ht->buckets = calloc(initial_capacity, sizeof *ht->buckets);
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }

    ht->capacity = initial_capacity;
    atomic_init(&ht->size, 0);
    return ht;
}

void ht_destroy(HashTable *ht)
{
    if (!ht) return;

    for (size_t i = 0; i < ht->capacity; i++) {
        HTEntry *e = ht->buckets[i].next;
        while (e) {
            HTEntry *next = e->next;
            free(e->key);
            free(e->value);
            free(e);
            e = next;
        }
    }

    free(ht->buckets);
    free(ht);
}

bool ht_set_ttl(HashTable *ht, const char *key, const char *value, int64_t expire_at_ms)
{
    if (!ht || !key || !value) return false;

    HTEntry *existing = ht_find_entry(ht, key);
    if (existing) {
        char *nv = xstrdup(value);
        if (!nv) return false;
        free(existing->value);
        existing->value = nv;
        existing->expire_at_ms = expire_at_ms;
        return true;
    }

    HTEntry *ne = malloc(sizeof *ne);
    if (!ne) return false;

    ne->key          = xstrdup(key);
    ne->value        = xstrdup(value);
    ne->expire_at_ms = expire_at_ms;
    if (!ne->key || !ne->value) {
        free(ne->key);
        free(ne->value);
        free(ne);
        return false;
    }

    size_t idx = (size_t)(hash_djb2(key) % ht->capacity);
    ne->next = ht->buckets[idx].next;
    ht->buckets[idx].next = ne;
    atomic_fetch_add(&ht->size, 1);
    return true;
}

bool ht_set(HashTable *ht, const char *key, const char *value)
{
    return ht_set_ttl(ht, key, value, 0);
}

char *ht_get(HashTable *ht, const char *key)
{
    if (!ht || !key) return NULL;
    HTEntry *e = ht_find_entry(ht, key);
    return e ? e->value : NULL;
}

bool ht_del(HashTable *ht, const char *key)
{
    if (!ht || !key) return false;

    size_t   idx  = (size_t)(hash_djb2(key) % ht->capacity);
    HTEntry *prev = &ht->buckets[idx];

    for (HTEntry *e = prev->next; e != NULL; prev = e, e = e->next) {
        if (strcmp(e->key, key) == 0) {
            prev->next = e->next;
            free(e->key);
            free(e->value);
            free(e);
            atomic_fetch_sub(&ht->size, 1);
            return true;
        }
    }
    return false;
}

size_t ht_size(const HashTable *ht)
{
    return ht ? (size_t)atomic_load(&ht->size) : 0;
}
