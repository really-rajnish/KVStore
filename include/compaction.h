#ifndef COMPACTION_H
#define COMPACTION_H

#include <stdbool.h>
#include "kvstore.h"

/* Atomically compacts the active WAL:
   1. Rewrites current live key-value set to a temporary log (.compact.tmp)
   2. Syncs temporary file via fsync
   3. Closes current WAL descriptor
   4. Atomically replaces WAL using rename(2)
   5. Reopens compacted WAL in append mode */
bool compaction_run(KVStore *kv);

#endif /* COMPACTION_H */
