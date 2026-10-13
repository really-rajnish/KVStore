# Verification & Testing Strategy (Weeks 1 – 12)

**Course**: KL University — 25CS2104E OS Capstone (Project 19)

---

## 1. Test Suite Matrix

The project includes 10 standalone unit and integration test binaries:

| Binary | Source | Focus Area | Assertions | Status |
|:-------|:-------|:-----------|:----------:|:------:|
| `bin/test_hash_table` | `tests/test_hash_table.c` | Hash table CRUD, DJB2 collisions, dynamic resizing | 28 | **PASS** |
| `bin/test_kvstore` | `tests/test_kvstore.c` | Wrapper API, size tracking, basic mutations | 18 | **PASS** |
| `bin/test_parser` | `tests/test_parser.c` | Tokenizer, quoted arguments, escape sequences | 30 | **PASS** |
| `bin/test_protocol` | `tests/test_protocol.c` | TCP buffer framing, 1-byte chunking, pipelining | 56 | **PASS** |
| `bin/test_server_client` | `tests/test_server_client.c` | Multi-client TCP networking, end-to-end queries | 23 | **PASS** |
| `bin/test_signals` | `tests/test_signals.c` | Self-pipe trick, async-signal-safe SIGINT/SIGTERM | 10 | **PASS** |
| `bin/test_wal` | `tests/test_wal.c` | Append-only log, IEEE CRC32, torn write recovery | 20 | **PASS** |
| `bin/test_compaction` | `tests/test_compaction.c` | Atomic snapshotting, 98% WAL log size reduction | 30 | **PASS** |
| `bin/test_concurrency` | `tests/test_concurrency.c` | ThreadPool, 16 threads, Striped RW vs Global Mutex | 56 | **PASS** |
| `bin/test_ttl` | `tests/test_ttl.c` | Lazy eviction, active reaper thread, WAL replay | 36 | **PASS** |

**Total Assertions**: 307 across 10 test suites, **0 failures**.

---

## 2. Distinction Bar Automated Verification

### 2.1 Memory Leak Audit (Valgrind)
```bash
make valgrind
```
- **Flags**: `valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1`
- **Result**: `0 bytes in 0 blocks in use at exit`, `0 errors from 0 contexts` across all 10 test binaries.

### 2.2 Concurrency Data Race Audit (ThreadSanitizer)
```bash
make tsan
```
- **Flags**: `-fsanitize=thread -g -O1 -pthread`
- **Execution**: `setarch x86_64 -R ./bin/test_concurrency_tsan`
- **Result**: Zero data races, zero mutex order violations, 0 warnings.

### 2.3 Crash Consistency (`kill -9`)
```bash
make crash-test
```
- **Script**: `scripts/crash_test.sh`
- **Workflow**: Writes 100 keys -> Delivers sudden `kill -9` -> Restarts and replays WAL -> Verifies 100% key recovery -> Writes mutations and delivers second `kill -9` -> Restarts and verifies consistency.
- **Result**: **100% PASSED**.

### 2.4 Empirical Benchmarking
```bash
make bench
```
- Evaluates 8 worker threads across 240,000 operations.
- Confirms **4.54x throughput speedup** and **98.7% p99 latency reduction**.

### 2.5 Viva Demonstration Script
```bash
make demo
```
- Automates live server startup, client queries, TTL expiration, compaction, `kill -9` crash recovery, signal-safe shutdown, and concurrency benchmarking.
