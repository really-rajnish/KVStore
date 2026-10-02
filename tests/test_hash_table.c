#include <stdio.h>
#include <string.h>

#include "hash_table.h"

static int failures = 0;

#define CHECK(cond, msg)                                    \
    do {                                                    \
        if (cond) { printf("PASS  %s\n", (msg)); }          \
        else      { printf("FAIL  %s\n", (msg)); failures++; } \
    } while (0)

int main(void)
{
    HashTable *ht = ht_create(8);
    CHECK(ht != NULL,              "create table");
    CHECK(ht_size(ht) == 0,        "new table is empty");

    CHECK(ht_set(ht, "a", "1"),    "set a=1");
    CHECK(ht_set(ht, "b", "2"),    "set b=2");
    CHECK(ht_size(ht) == 2,        "size is 2");

    CHECK(strcmp(ht_get(ht, "a"), "1") == 0, "get a");
    CHECK(strcmp(ht_get(ht, "b"), "2") == 0, "get b");
    CHECK(ht_get(ht, "missing") == NULL,     "get missing -> NULL");

    CHECK(ht_set(ht, "a", "99"),   "overwrite a");
    CHECK(strcmp(ht_get(ht, "a"), "99") == 0, "a was overwritten");
    CHECK(ht_size(ht) == 2,        "overwrite did not grow size");

    CHECK(ht_del(ht, "a"),         "delete a");
    CHECK(ht_get(ht, "a") == NULL, "a is gone");
    CHECK(!ht_del(ht, "a"),        "double delete returns false");
    CHECK(ht_size(ht) == 1,        "size is 1");

    char k[32], v[32];
    for (int i = 0; i < 500; i++) {
        snprintf(k, sizeof k, "key%d", i);
        snprintf(v, sizeof v, "val%d", i);
        if (!ht_set(ht, k, v)) { printf("FAIL  insert key%d\n", i); failures++; }
    }
    CHECK(ht_size(ht) == 501,                              "size after growth");
    CHECK(strcmp(ht_get(ht, "key123"), "val123") == 0,     "lookup survives resize");
    CHECK(strcmp(ht_get(ht, "key499"), "val499") == 0,     "last key survives resize");

    for (int i = 0; i < 500; i++) {
        snprintf(k, sizeof k, "key%d", i);
        if (!ht_del(ht, k)) { printf("FAIL  delete key%d\n", i); failures++; }
    }
    CHECK(ht_size(ht) == 1,        "bulk delete leaves only b");
    CHECK(strcmp(ht_get(ht, "b"), "2") == 0, "b still present");

    ht_destroy(ht);

    printf("\n%s (%d failure%s)\n",
           failures ? "TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
