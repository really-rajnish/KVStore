#include <stdio.h>
#include <string.h>

#include "kvstore.h"

static int failures = 0;

#define CHECK(cond, msg)                                         \
    do {                                                         \
        if (cond) { printf("PASS  %s\n", (msg)); }               \
        else      { printf("FAIL  %s\n", (msg)); failures++; }   \
    } while (0)

int main(void)
{
    KVStore *kv = kvstore_create();
    CHECK(kv != NULL,                              "create");
    CHECK(kvstore_size(kv) == 0,                   "new store empty");

    CHECK(kvstore_set(kv, "a", "1"),               "set a=1");
    CHECK(strcmp(kvstore_get(kv, "a"), "1") == 0,  "get a");
    CHECK(kvstore_size(kv) == 1,                   "size 1");

    CHECK(kvstore_set(kv, "a", "2"),               "overwrite a");
    CHECK(strcmp(kvstore_get(kv, "a"), "2") == 0,  "get a after overwrite");
    CHECK(kvstore_size(kv) == 1,                   "overwrite does not grow");

    CHECK(kvstore_get(kv, "missing") == NULL,      "missing -> NULL");
    CHECK(kvstore_del(kv, "a"),                    "del a");
    CHECK(kvstore_get(kv, "a") == NULL,            "a gone");
    CHECK(!kvstore_del(kv, "a"),                   "double del returns false");

    kvstore_destroy(kv);

    printf("\n%s (%d failure%s)\n",
           failures ? "TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
