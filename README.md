# KVStore: High-Performance Concurrent Key-Value Store

An industrial-grade, crash-consistent, multi-threaded in-memory Key-Value store daemon written in **C11** with zero third-party dependencies (POSIX pthreads and sockets only).

Developed for **KL University — 25CS2104E Operating Systems Capstone (Project 19)** to satisfy the **Distinction Bar**.

---

## Features & Distinction Bar Deliverables

- **In-Memory Index**: High-performance separate chaining hash table with DJB2 hashing and automatic load-factor expansion ($O(1)$ lookup/insert).
- **TCP Networking**: Non-blocking line/stream protocol engine tolerating 1-byte fragmented reads, partial buffers, and pipelined bursts.
- **Signal-Safe Graceful Shutdown**: Implements the **Self-Pipe Trick** (`self_pipe[2]`) with connection draining on `SIGINT` and `SIGTERM`.
- **Write-Ahead Log (WAL)**: Binary record framing protected by IEEE 802.3 CRC32 checksums, configurable `fsync` policies, and startup recovery that rejects torn writes.
- **Crash Consistency (`kill -9`)**: Verified recovery surviving abrupt `SIGKILL` without data loss or corruption (`make crash-test`).
- **Online Log Compaction**: Atomic live snapshotting via temporary log creation and POSIX `rename(2)` swap, achieving 98%+ WAL disk reduction while remaining accessible to clients.
- **Fine-Grained Concurrency**: Striped reader-writer locking with 32 independent `pthread_rwlock_t` instances and thread-safe copy reads, delivering a **4.54x throughput speedup** over a global mutex.
- **Key Expiry & TTL Subsystem**: Sub-millisecond expiration engine supporting both *lazy eviction on access* and an *active background reaper thread*.
- **Memory Discipline & Race Freedom**: **Valgrind clean** (0 bytes leaked in 0 blocks) and **ThreadSanitizer clean** (0 data races).

---

## Quick Start

### 1. Build All Binaries
```bash
make clean
make all
```
Generates binaries in `bin/`:
- `bin/kvstore`: Interactive REPL CLI
- `bin/kvstore_server`: Multi-threaded TCP daemon
- `bin/kvstore_client`: TCP client utility
- `bin/kvstore_bench`: Concurrency benchmark tool

### 2. Run the Full Test Suite
```bash
make test
```
Executes all 10 test suites (307 assertions, 0 failures).

### 3. Run the Live Viva Demonstration
```bash
make demo
```
Walks through the entire capstone deliverable set in an automated live presentation format.

### 4. Run Memory & Concurrency Audits
```bash
make valgrind     # Full leak-check across all 10 binaries (0 bytes leaked)
make tsan         # ThreadSanitizer data-race audit (0 data races)
make crash-test   # Automated kill -9 durability test
make bench        # Concurrency throughput and latency benchmarks
```

---

## Command Reference

| Command | Syntax | Description | Example |
|:---|:---|:---|:---|
| **SET** | `SET <key> <val> [ttl_ms]` | Insert/update key with optional TTL in ms | `SET user:1 "Alice Smith" 5000` |
| **GET** | `GET <key>` | Retrieve value or `(nil)` | `GET user:1` |
| **DEL** | `DEL <key>` | Delete key (`OK` or `NOT_FOUND`) | `DEL user:1` |
| **COUNT**| `COUNT` | Return count of live keys | `COUNT` |
| **EXPIRE**| `EXPIRE <key> <ttl_ms>` | Set TTL on existing key | `EXPIRE user:1 1000` |
| **TTL** | `TTL <key>` | Query remaining TTL (-1: eternal, -2: absent) | `TTL user:1` |
| **COMPACT**| `COMPACT` | Trigger online log compaction | `COMPACT` |
| **QUIT** | `QUIT` / `EXIT` | Close client connection | `QUIT` |

---

## Project Structure

```
.
├── Makefile
├── README.md
├── bin/                 # Compiled binaries
├── data/                # Active WAL files
├── docs/                # Architecture, design, testing & benchmark docs
│   ├── architecture.md
│   ├── benchmark.md
│   ├── design.md
│   ├── requirements.md
│   └── testing.md
├── include/             # C header interfaces
│   ├── client.h
│   ├── compaction.h
│   ├── dispatcher.h
│   ├── hash_table.h
│   ├── kvstore.h
│   ├── parser.h
│   ├── protocol.h
│   ├── server.h
│   ├── threadpool.h
│   ├── ttl.h
│   └── wal.h
├── scripts/             # Automated test & demo scripts
│   ├── benchmark.sh
│   ├── crash_test.sh
│   └── demo.sh
├── src/                 # Implementation sources
│   ├── bench_main.c
│   ├── client.c
│   ├── client_main.c
│   ├── compaction.c
│   ├── dispatcher.c
│   ├── hash_table.c
│   ├── kvstore.c
│   ├── main.c
│   ├── parser.c
│   ├── protocol.c
│   ├── server.c
│   ├── server_main.c
│   ├── threadpool.c
│   ├── ttl.c
│   └── wal.c
└── tests/               # 10 Standalone unit & integration test suites
    ├── test_compaction.c
    ├── test_concurrency.c
    ├── test_hash_table.c
    ├── test_kvstore.c
    ├── test_parser.c
    ├── test_protocol.c
    ├── test_server_client.c
    ├── test_signals.c
    ├── test_ttl.c
    └── test_wal.c
```
