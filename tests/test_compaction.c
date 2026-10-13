#include "kvstore.h"
#include "compaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static int total_asserts = 0;
static int failed_asserts = 0;

#define TEST_ASSERT(cond, msg) do { \
    total_asserts++; \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        failed_asserts++; \
    } else { \
        printf("PASS: %s\n", msg); \
    } \
    fflush(stdout); \
} while(0)

static off_t get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

static void test_compaction_shrinkage(void)
{
    printf("\n--- Test 1: Compaction shrinks log size ---\n");
    const char *path = "data/test_compaction.wal";
    unlink(path);

    KVStore *kv = kvstore_create_with_wal(path, WAL_SYNC_NEVER);
    TEST_ASSERT(kv != NULL, "create KVStore with WAL for compaction");

    /* Insert 10 unique keys and overwrite them 50 times each (500 log records) */
    char key[32], val[32];
    for (int round = 0; round < 50; round++) {
        for (int i = 0; i < 10; i++) {
            snprintf(key, sizeof(key), "key:%d", i);
            snprintf(val, sizeof(val), "val_round_%d", round);
            kvstore_set(kv, key, val);
        }
    }

    off_t size_before = get_file_size(path);
    TEST_ASSERT(size_before > 10000, "uncompacted WAL grew > 10KB");
    printf("Info: WAL size before compaction: %ld bytes\n", (long)size_before);

    /* Run compaction */
    TEST_ASSERT(kvstore_compact(kv), "kvstore_compact() succeeded");

    off_t size_after = get_file_size(path);
    printf("Info: WAL size after compaction: %ld bytes\n", (long)size_after);
    TEST_ASSERT(size_after > 0 && size_after < size_before / 5, "compacted WAL shrunk by > 80%");

    /* Assert all 10 live keys retained correct final values */
    TEST_ASSERT(kvstore_size(kv) == 10, "live size is still 10");
    for (int i = 0; i < 10; i++) {
        snprintf(key, sizeof(key), "key:%d", i);
        const char *v = kvstore_get(kv, key);
        TEST_ASSERT(v != NULL && strcmp(v, "val_round_49") == 0, "live key value preserved");
    }

    kvstore_destroy(kv);

    /* Re-open from compacted log and assert complete recovery */
    KVStore *kv_reopen = kvstore_create_with_wal(path, WAL_SYNC_NEVER);
    TEST_ASSERT(kv_reopen != NULL, "reopen from compacted WAL");
    TEST_ASSERT(kvstore_size(kv_reopen) == 10, "reopened size is 10");
    for (int i = 0; i < 10; i++) {
        snprintf(key, sizeof(key), "key:%d", i);
        const char *v = kvstore_get(kv_reopen, key);
        TEST_ASSERT(v != NULL && strcmp(v, "val_round_49") == 0, "reopened key value matches");
    }

    /* Verify store continues accepting mutations after compaction */
    TEST_ASSERT(kvstore_set(kv_reopen, "key:new", "hello"), "insert key after compaction");
    TEST_ASSERT(kvstore_del(kv_reopen, "key:0"), "delete key after compaction");
    TEST_ASSERT(kvstore_size(kv_reopen) == 10, "size after insert + delete is 10");

    kvstore_destroy(kv_reopen);
    unlink(path);
}

int main(void)
{
    printf("Running Log Compaction tests (Week 9)...\n");
    test_compaction_shrinkage();

    if (failed_asserts == 0) {
        printf("\nALL TESTS PASSED (%d assertions, 0 failures)\n", total_asserts);
        return 0;
    } else {
        printf("\nTESTS FAILED (%d failures out of %d assertions)\n", failed_asserts, total_asserts);
        return 1;
    }
}
