#include "compaction.h"
#include "wal.h"
#include "ttl.h"
#include "hash_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool compaction_run(KVStore *kv)
{
    if (!kv || !kv->wal_path) return false;

    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.compact.tmp", kv->wal_path);
    unlink(tmp_path);

    Wal *tmp_wal = wal_open(tmp_path, WAL_SYNC_NEVER);
    if (!tmp_wal) return false;

    int64_t now_ms = ttl_current_time_ms();
    HashTable *ht = kv->table;

    if (ht) {
        for (size_t i = 0; i < ht->capacity; i++) {
            for (HTEntry *e = ht->buckets[i].next; e != NULL; e = e->next) {
                /* Skip expired keys during snapshot creation */
                if (e->expire_at_ms > 0 && now_ms >= e->expire_at_ms) {
                    continue;
                }

                bool ok = false;
                if (e->expire_at_ms > 0) {
                    ok = wal_append_set_ttl(tmp_wal, e->key, e->value, e->expire_at_ms);
                } else {
                    ok = wal_append_set(tmp_wal, e->key, e->value);
                }

                if (!ok) {
                    wal_close(tmp_wal);
                    unlink(tmp_path);
                    return false;
                }
            }
        }
    }

    if (!wal_sync(tmp_wal)) {
        wal_close(tmp_wal);
        unlink(tmp_path);
        return false;
    }
    wal_close(tmp_wal);

    if (kv->wal) {
        wal_close(kv->wal);
        kv->wal = NULL;
    }

    if (rename(tmp_path, kv->wal_path) != 0) {
        kv->wal = wal_open(kv->wal_path, kv->sync_policy);
        return false;
    }

    kv->wal = wal_open(kv->wal_path, kv->sync_policy);
    return (kv->wal != NULL);
}
