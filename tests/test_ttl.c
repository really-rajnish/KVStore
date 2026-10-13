#include "kvstore.h"
#include "ttl.h"
#include "wal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int total_asserts = 0;
static int failed_asserts = 0;

#define TEST_ASSERT(cond, msg) do {     total_asserts++;     if (!(cond)) {         printf("FAIL: %s (line %d)\n", msg, __LINE__);         failed_asserts++;     } else {         printf("PASS: %s\n", msg);     }     fflush(stdout); } while(0)

static void test_lazy_eviction(void)
{
    printf("\n--- Test 1: Lazy Eviction on Read ---\n");
    KVStore *kv = kvstore_create();
    TEST_ASSERT(kv != NULL, "create KVStore");

    TEST_ASSERT(kvstore_set_ttl(kv, "temp_key", "temporary_val", 100), "set key with 100ms TTL");
    TEST_ASSERT(kvstore_size(kv) == 1, "store size is 1");

    char buf[64];
    TEST_ASSERT(kvstore_get_copy(kv, "temp_key", buf, sizeof(buf)), "get key before expiry succeeds");
    TEST_ASSERT(strcmp(buf, "temporary_val") == 0, "value matches before expiry");

    int64_t ttl_val = kvstore_ttl(kv, "temp_key");
    TEST_ASSERT(ttl_val > 0 && ttl_val <= 100, "TTL query returns remaining ms > 0");

    /* Sleep 150ms to ensure expiration */
    usleep(150000);

    TEST_ASSERT(!kvstore_get_copy(kv, "temp_key", buf, sizeof(buf)), "get key after expiry returns false (nil)");
    TEST_ASSERT(kvstore_ttl(kv, "temp_key") == -2, "TTL after expiry returns -2 (not found)");
    TEST_ASSERT(kvstore_size(kv) == 0, "size decremented after lazy eviction to 0");

    kvstore_destroy(kv);
}

static void test_expire_command(void)
{
    printf("\n--- Test 2: EXPIRE command and TTL status ---\n");
    KVStore *kv = kvstore_create();

    TEST_ASSERT(kvstore_set(kv, "perm_key", "eternal"), "set persistent key without TTL");
    TEST_ASSERT(kvstore_ttl(kv, "perm_key") == -1, "TTL of persistent key returns -1 (no expiry)");

    TEST_ASSERT(kvstore_expire(kv, "perm_key", 120), "expire key with 120ms TTL");
    int64_t rem = kvstore_ttl(kv, "perm_key");
    TEST_ASSERT(rem > 0 && rem <= 120, "TTL updated to positive remaining ms");

    TEST_ASSERT(!kvstore_expire(kv, "nonexistent", 500), "expire on non-existent key returns false");

    usleep(160000);
    TEST_ASSERT(kvstore_ttl(kv, "perm_key") == -2, "key expired after 160ms");
    TEST_ASSERT(kvstore_size(kv) == 0, "store size is 0");

    kvstore_destroy(kv);
}

static void test_active_reaper(void)
{
    printf("\n--- Test 3: Active Background Reaper Thread ---\n");
    KVStore *kv = kvstore_create();

    TEST_ASSERT(kvstore_start_ttl_reaper(kv, 30), "start active background reaper with 30ms sweep");

    /* Insert 30 keys with short 80ms TTL */
    for (int i = 0; i < 30; i++) {
        char k[32], v[32];
        snprintf(k, sizeof(k), "reap_key:%d", i);
        snprintf(v, sizeof(v), "val:%d", i);
        kvstore_set_ttl(kv, k, v, 80);
    }

    TEST_ASSERT(kvstore_size(kv) == 30, "all 30 keys inserted");

    /* Wait 200ms without touching any keys with GET */
    usleep(200000);

    /* Background reaper must have actively purged expired keys */
    TEST_ASSERT(kvstore_size(kv) == 0, "all 30 keys actively purged by background reaper");

    kvstore_destroy(kv);
}

static void test_ttl_wal_recovery(void)
{
    printf("\n--- Test 4: TTL Durability and WAL Replay ---\n");
    const char *wal_path = "data/test_ttl.wal";
    unlink(wal_path);

    KVStore *kv1 = kvstore_create_with_wal(wal_path, WAL_SYNC_NEVER);
    TEST_ASSERT(kv1 != NULL, "create KVStore with WAL");

    /* Key 1: long lived (10 seconds) */
    TEST_ASSERT(kvstore_set_ttl(kv1, "long_lived", "survivor", 10000), "set long-lived key (10s)");
    /* Key 2: short lived (60ms) */
    TEST_ASSERT(kvstore_set_ttl(kv1, "short_lived", "ephemeral", 60), "set short-lived key (60ms)");
    /* Key 3: indefinite */
    TEST_ASSERT(kvstore_set(kv1, "indefinite", "forever"), "set indefinite key");

    /* Sleep 100ms so short_lived expires */
    usleep(100000);

    kvstore_destroy(kv1);

    /* Reopen store from WAL */
    KVStore *kv2 = kvstore_create_with_wal(wal_path, WAL_SYNC_NEVER);
    TEST_ASSERT(kv2 != NULL, "reopen KVStore from WAL");

    char buf[64];
    TEST_ASSERT(kvstore_get_copy(kv2, "long_lived", buf, sizeof(buf)), "long-lived key recovered");
    TEST_ASSERT(strcmp(buf, "survivor") == 0, "long-lived value matches");
    TEST_ASSERT(kvstore_ttl(kv2, "long_lived") > 0, "long-lived key has remaining TTL > 0");

    TEST_ASSERT(!kvstore_get_copy(kv2, "short_lived", buf, sizeof(buf)), "short-lived key rejected on replay");
    TEST_ASSERT(kvstore_ttl(kv2, "short_lived") == -2, "short-lived key returns -2");

    TEST_ASSERT(kvstore_get_copy(kv2, "indefinite", buf, sizeof(buf)), "indefinite key recovered");
    TEST_ASSERT(kvstore_ttl(kv2, "indefinite") == -1, "indefinite key returns -1");

    kvstore_destroy(kv2);
    unlink(wal_path);
}

static void test_ttl_compaction(void)
{
    printf("\n--- Test 5: Log Compaction Drops Expired Keys ---\n");
    const char *wal_path = "data/test_ttl_compact.wal";
    unlink(wal_path);

    KVStore *kv = kvstore_create_with_wal(wal_path, WAL_SYNC_NEVER);
    kvstore_set_ttl(kv, "temp1", "val1", 50);
    kvstore_set_ttl(kv, "temp2", "val2", 50);
    kvstore_set(kv, "keep", "survives");

    usleep(80000);

    TEST_ASSERT(kvstore_compact(kv), "kvstore_compact() succeeds with expired keys");

    kvstore_destroy(kv);

    KVStore *reopened = kvstore_create_with_wal(wal_path, WAL_SYNC_NEVER);
    TEST_ASSERT(kvstore_size(reopened) == 1, "compacted WAL only has 1 live key");
    char b[64];
    TEST_ASSERT(kvstore_get_copy(reopened, "keep", b, sizeof(b)), "surviving key retrieved");
    TEST_ASSERT(!kvstore_get_copy(reopened, "temp1", b, sizeof(b)), "expired temp1 not in compacted WAL");
    TEST_ASSERT(!kvstore_get_copy(reopened, "temp2", b, sizeof(b)), "expired temp2 not in compacted WAL");

    kvstore_destroy(reopened);
    unlink(wal_path);
}

int main(void)
{
    printf("Running Key Expiry & TTL Subsystem tests (Week 11)...\n");
    test_lazy_eviction();
    test_expire_command();
    test_active_reaper();
    test_ttl_wal_recovery();
    test_ttl_compaction();

    if (failed_asserts == 0) {
        printf("\nALL TTL TESTS PASSED (%d assertions, 0 failures)\n", total_asserts);
        return 0;
    } else {
        printf("\nTTL TESTS FAILED (%d failures out of %d assertions)\n", failed_asserts, total_asserts);
        return 1;
    }
}
