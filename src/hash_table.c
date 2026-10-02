#include "hash_table.h"

#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define MAX_LOAD_FACTOR  0.75
#define MIN_CAPACITY     8

/* ---------- helpers ---------- */

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char  *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* djb2 — Bernstein's classic string hash. */
static unsigned long hash_djb2(const char *s)
{
    unsigned long h = 5381UL;
    int c;
    while ((c = (unsigned char)*s++) != 0)
        h = ((h << 5) + h) + (unsigned long)c;
    return h;
}

static HTEntry *find_entry(const HashTable *ht, const char *key)
{
    size_t idx = (size_t)(hash_djb2(key) % ht->capacity);
    for (HTEntry *e = ht->buckets[idx].next; e != NULL; e = e->next)
        if (strcmp(e->key, key) == 0)
            return e;
    return NULL;
}

/* Rehash every entry into a new bucket array. */
static bool ht_resize(HashTable *ht, size_t new_capacity)
{
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

/* ---------- public API ---------- */

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
    ht->size     = 0;
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

bool ht_set(HashTable *ht, const char *key, const char *value)
{
    if (!ht || !key || !value) return false;

    if ((double)(ht->size + 1) / (double)ht->capacity > MAX_LOAD_FACTOR) {
        if (!ht_resize(ht, ht->capacity * 2))
            return false;
    }

    HTEntry *existing = find_entry(ht, key);
    if (existing) {
        char *nv = xstrdup(value);
        if (!nv) return false;
        free(existing->value);
        existing->value = nv;
        return true;
    }

    HTEntry *ne = malloc(sizeof *ne);
    if (!ne) return false;

    ne->key   = xstrdup(key);
    ne->value = xstrdup(value);
    if (!ne->key || !ne->value) {
        free(ne->key);
        free(ne->value);
        free(ne);
        return false;
    }

    size_t idx = (size_t)(hash_djb2(key) % ht->capacity);
    ne->next = ht->buckets[idx].next;
    ht->buckets[idx].next = ne;
    ht->size++;
    return true;
}

char *ht_get(HashTable *ht, const char *key)
{
    if (!ht || !key) return NULL;
    HTEntry *e = find_entry(ht, key);
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
            ht->size--;
            return true;
        }
    }
    return false;
}

size_t ht_size(const HashTable *ht)
{
    return ht ? ht->size : 0;
}
