#ifndef TTL_H
#define TTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct KVStore KVStore;
typedef struct TTLReaper TTLReaper;

int64_t ttl_current_time_ms(void);

TTLReaper *ttl_reaper_start(KVStore *kv, int interval_ms);
void       ttl_reaper_stop(TTLReaper *reaper);
size_t     ttl_evict_expired(KVStore *kv);

#endif /* TTL_H */
