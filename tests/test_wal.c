#include "wal.h"
#include "kvstore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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

static void test_wal_append_and_replay(void)
{
    printf("\n--- Test 1: Basic WAL append and replay ---\n");
    const char *path = "data/test_wal1.wal";
    unlink(path);

    KVStore *kv1 = kvstore_create_with_wal(path, WAL_SYNC_ALWAYS);
    TEST_ASSERT(kv1 != NULL, "create KVStore with WAL");

    TEST_ASSERT(kvstore_set(kv1, "user:1", "alice"), "set user:1");
    TEST_ASSERT(kvstore_set(kv1, "user:2", "bob"), "set user:2");
    TEST_ASSERT(kvstore_set(kv1, "user:3", "charlie"), "set user:3");
    TEST_ASSERT(kvstore_set(kv1, "user:1", "alice_updated"), "overwrite user:1");
    TEST_ASSERT(kvstore_del(kv1, "user:2"), "del user:2");
    TEST_ASSERT(kvstore_size(kv1) == 2, "size before shutdown is 2");

    kvstore_destroy(kv1);

    /* Re-open in a fresh KVStore and assert state is recovered */
    KVStore *kv2 = kvstore_create_with_wal(path, WAL_SYNC_ALWAYS);
    TEST_ASSERT(kv2 != NULL, "reopen KVStore from WAL");

    TEST_ASSERT(kvstore_size(kv2) == 2, "recovered size is 2");
    TEST_ASSERT(strcmp(kvstore_get(kv2, "user:1"), "alice_updated") == 0, "user:1 value recovered");
    TEST_ASSERT(strcmp(kvstore_get(kv2, "user:3"), "charlie") == 0, "user:3 value recovered");
    TEST_ASSERT(kvstore_get(kv2, "user:2") == NULL, "deleted user:2 is absent");

    kvstore_destroy(kv2);
    unlink(path);
}

static void test_torn_write_recovery(void)
{
    printf("\n--- Test 2: Torn write recovery and CRC validation ---\n");
    const char *path = "data/test_wal_torn.wal";
    unlink(path);

    KVStore *kv1 = kvstore_create_with_wal(path, WAL_SYNC_ALWAYS);
    kvstore_set(kv1, "item:1", "100");
    kvstore_set(kv1, "item:2", "200");
    kvstore_destroy(kv1);

    /* Corrupt the end of the file by appending half a record */
    int fd = open(path, O_WRONLY | O_APPEND);
    TEST_ASSERT(fd >= 0, "open WAL for corruption injection");
    char junk[] = "\x01\x00\x05\x00\x00\x00"; /* partial header, missing key/val/crc */
    ssize_t w1 = write(fd, junk, sizeof(junk)); (void)w1;
    close(fd);

    /* Replay must recover the 2 valid items and cleanly stop without crash */
    KVStore *kv2 = kvstore_create_with_wal(path, WAL_SYNC_ALWAYS);
    TEST_ASSERT(kv2 != NULL, "reopen corrupted WAL");
    TEST_ASSERT(kvstore_size(kv2) == 2, "recovered exactly 2 valid items");
    TEST_ASSERT(strcmp(kvstore_get(kv2, "item:1"), "100") == 0, "item:1 present");
    TEST_ASSERT(strcmp(kvstore_get(kv2, "item:2"), "200") == 0, "item:2 present");

    kvstore_destroy(kv2);
    unlink(path);
}

static void test_crc_mismatch_recovery(void)
{
    printf("\n--- Test 3: Bad CRC detection ---\n");
    const char *path = "data/test_wal_badcrc.wal";
    unlink(path);

    KVStore *kv1 = kvstore_create_with_wal(path, WAL_SYNC_ALWAYS);
    kvstore_set(kv1, "k1", "v1");
    kvstore_set(kv1, "k2", "v2");
    kvstore_destroy(kv1);

    /* Tamper with a payload byte in the file */
    int fd = open(path, O_RDWR);
    TEST_ASSERT(fd >= 0, "open WAL for bit tampering");
    lseek(fd, 8, SEEK_SET); /* corrupt byte in payload */
    char bad = 'X';
    ssize_t w2 = write(fd, &bad, 1); (void)w2;
    close(fd);

    /* Replay should stop at bad CRC */
    KVStore *kv2 = kvstore_create_with_wal(path, WAL_SYNC_ALWAYS);
    TEST_ASSERT(kv2 != NULL, "open tampered WAL");
    /* The first record was tampered, so it is rejected */
    TEST_ASSERT(kvstore_size(kv2) == 0, "tampered record rejected by CRC");

    kvstore_destroy(kv2);
    unlink(path);
}

int main(void)
{
    printf("Running Durability & Replay tests (Week 7)...\n");
    test_wal_append_and_replay();
    test_torn_write_recovery();
    test_crc_mismatch_recovery();

    if (failed_asserts == 0) {
        printf("\nALL TESTS PASSED (%d assertions, 0 failures)\n", total_asserts);
        return 0;
    } else {
        printf("\nTESTS FAILED (%d failures out of %d assertions)\n", failed_asserts, total_asserts);
        return 1;
    }
}
