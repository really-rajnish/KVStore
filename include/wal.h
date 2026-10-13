#ifndef WAL_H
#define WAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "kvstore.h"

#define WAL_OP_SET     0x01
#define WAL_OP_DEL     0x02
#define WAL_OP_SET_TTL 0x03

typedef struct Wal Wal;

Wal    *wal_open(const char *filepath, WalSyncPolicy sync_policy);
void    wal_close(Wal *wal);

bool    wal_append_set(Wal *wal, const char *key, const char *value);
bool    wal_append_set_ttl(Wal *wal, const char *key, const char *value, int64_t expire_at_ms);
bool    wal_append_del(Wal *wal, const char *key);
bool    wal_sync(Wal *wal);

size_t  wal_replay(KVStore *kv, const char *filepath);
uint32_t wal_crc32(const void *data, size_t len);

#endif /* WAL_H */
