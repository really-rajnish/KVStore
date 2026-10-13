# System Architecture (Weeks 1 – 12)

**Course**: KL University — 25CS2104E OS Capstone (Project 19)  
**Standard**: ISO/IEC 9899:2011 (C11)

---

## 1. High-Level System Architecture

```
                          +-------------------------+
                          |   Interactive REPL CLI  | (bin/kvstore)
                          +------------+------------+
                                       |
+--------------------------+           |
|      TCP Client CLI      |           |
|   (bin/kvstore_client)   |           |
+------------+-------------+           |
             |                         |
             | [TCP Socket Stream]     |
             v                         |
+--------------------------+           |
|    TCP Server Daemon     |           |
|   (bin/kvstore_server)   |           |
|                          |           |
|  - Self-Pipe Trick       |           |
|  - Connection Tracking   |           |
|  - Drain on SIGINT/TERM  |           |
+------------+-------------+           |
             |                         |
             +------------+------------+
                          |
                          v
               +---------------------+
               | Protocol & Parser   | (protocol.c / parser.c)
               |  - Line framing     |
               |  - Tokenizer        |
               +----------+----------+
                          |
                          v
               +---------------------+
               |     Dispatcher      | (dispatcher.c)
               |  - SET, GET, DEL    |
               |  - COUNT, EXPIRE    |
               |  - TTL, COMPACT     |
               +----------+----------+
                          |
                          v
+-------------------------------------------------------------+
|                     KVStore Engine Layer                    |
|                                                             |
|  +-------------------------------------------------------+  |
|  |           Concurrency Subsystem (Week 10)             |  |
|  |   - LockMode: KV_LOCK_GLOBAL vs KV_LOCK_STRIPED       |  |
|  |   - 32 Striped pthread_rwlock_t instances             |  |
|  |   - Atomic table size tracking (stdatomic.h)          |  |
|  +-------------------------------------------------------+  |
|                                                             |
|  +-------------------------------------------------------+  |
|  |          Key Expiry & TTL Subsystem (Week 11)         |  |
|  |   - Lazy eviction on read (kvstore_get_copy)          |  |
|  |   - Background active reaper thread (ttl.c)           |  |
|  +-------------------------------------------------------+  |
|                                                             |
|  +---------------------------+ +-------------------------+  |
|  | In-Memory Hash Table      | | Write-Ahead Log (WAL)   |  |
|  |  - Separate chaining      | |  - Binary record format |  |
|  |  - djb2 hash function     | |  - IEEE 802.3 CRC32     |  |
|  |  - Power-of-2 auto resize | |  - fsync durability     |  |
|  |  - Sentinel head nodes    | |  - Crash replay engine  |  |
|  +---------------------------+ +-------------------------+  |
|                                                             |
|  +-------------------------------------------------------+  |
|  |         Online Log Compaction Engine (Week 9)         |  |
|  |   - Live snapshotting to .compact.tmp                 |  |
|  |   - Expired key pruning                               |  |
|  |   - Atomic rename(2) log replacement                  |  |
|  +-------------------------------------------------------+  |
+-------------------------------------------------------------+
```

---

## 2. Core Subsystems

### 2.1 Networking & Signal Handling (Weeks 5–6)
- **Protocol**: Dynamic buffer management (`ClientBuffer`) supporting partial reads, 1-byte chunk fragmentation, and pipelined bursts.
- **Self-Pipe Trick**: A dedicated pipe (`self_pipe[2]`) allows asynchronous signal handlers (`SIGINT`/`SIGTERM`) to wake up `poll()` in the accept loop safely without signal-unsafe library calls.
- **Connection Draining**: Shuts down client descriptors with `shutdown(fd, SHUT_RDWR)` and drains in-flight requests gracefully using condition variables and timeouts.

### 2.2 Durability & Crash Recovery (Weeks 7–8)
- **WAL Record Format**:
  ```
  +---------+------------+------------+---------------+-----------------+-----------+
  | Op (1B) | KeyLen(2B) | ValLen(4B) | Key (KeyLenB) | Val (ValLenB)   | CRC32(4B) |
  +---------+------------+------------+---------------+-----------------+-----------+
  ```
- **Crash Consistency**: Guaranteed survival against sudden `kill -9` (`SIGKILL`). Replay reads sequentially, validates 32-bit CRC against bit corruption, and halts at any torn write without crashing.

### 2.3 Online Atomic Compaction (Week 9)
- Minimally-blocking live snapshot generation.
- Iterates live keys, skips expired entries, writes to `<wal_path>.compact.tmp`, forces `fsync()`, closes, and swaps into place via atomic `rename(2)`.

### 2.4 Striped Locking & Concurrency Subsystem (Week 10)
- **Striping**: Table divided across 32 independent reader-writer locks (`pthread_rwlock_t`).
- **Bucket-Stripe Mapping**: `stripe_idx = (hash_djb2(key) % capacity) % 32`. Guarantees that any bucket's collision chain is protected by exactly one RW-lock.
- **TSan-Clean**: All shared metadata (`ht->size`) uses C11 `stdatomic.h`.

### 2.5 TTL & Key Expiry Engine (Week 11)
- **Dual-Mode Eviction**:
  1. *Lazy Eviction*: Triggered on read access. If expired, upgrades to write lock, frees memory, and writes deletion to WAL.
  2. *Active Background Reaper*: A dedicated daemon thread waking every 100ms to scan buckets and purge abandoned expired keys.
