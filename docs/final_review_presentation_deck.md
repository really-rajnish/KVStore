# OS Capstone Final Review: Complete Presentation Deck (Weeks 1 to 12) & Defense Guide

**Course**: KL University — 25CS2104E Operating Systems Capstone  
**Approved Use Case**: Project 19 — High-Performance Concurrent Key-Value Store  
**Deliverable**: 28-Slide Master Final Defense Presentation Deck & Viva Defense Script  
**PowerPoint File**: [`docs/OS_Capstone_Final_Review_Weeks_1_to_12_KVStore.pptx`](file:///Ubuntu/root/KVStore/docs/OS_Capstone_Final_Review_Weeks_1_to_12_KVStore.pptx)  
**Repository**: [github.com/really-rajnish/KVStore](https://github.com/really-rajnish/KVStore)

---

## Capstone Team (Project 19)
- **Rajnish Kumar Ranjan** · Reg No: `2500032608` (Project Lead: Hash Table Index, Striped Concurrency Architecture, Compaction Engine)
- **Hardik Gupta** · Reg No: `2500032616` (Durability Lead: Write-Ahead Log Framing, IEEE CRC32 Checksums, Crash Recovery State Machine)
- **Om Amrit Das** · Reg No: `2500032631` (Networking Lead: Non-Blocking TCP Daemon, Self-Pipe Trick, Streaming Protocol & Client CLI)

---

## 28-Slide Presentation Structure (Weeks 1 to 12)

| Slide # | Slide Title | Phase / Focus | Key Deliverable Covered |
|:---:|:---|:---|:---|
| **01** | Cover Slide & Team Identification | Executive | Project Title, Subtitle, GitHub URL, 3 Team Members |
| **02** | Project Abstract & Distinction Deliverables | Executive | C11 Daemon, Zero Dependencies, Distinction Bar Summary |
| **03** | Chronological 12-Week Roadmap | Planning | Weeks 1–12 4-Phase Progression (Index, Net, WAL, Concurrency) |
| **04** | Problem Statement & Systems Engineering Dilemma | Problem | Memory Volatility vs. Disk I/O, Lock Contention, kill -9 Corruption |
| **05** | Project Objectives, Scope & Constraints | Scope | O(1) Index, Crash Consistency, Striped Locks, Boundaries |
| **06** | Comprehensive 4-Tier System Architecture | Design | Client CLI, Network Daemon, Protocol/Concurrency, Storage/WAL |
| **07** | End-to-End Query Processing Lifecycle | Design | Trace: Ingress $\to$ Framing $\to$ Parsing $\to$ WAL $\to$ Lock $\to$ Reply |
| **08** | In-Memory Index Design & DJB2 Hashing | Phase I (W1–W2) | Separate Chaining, DJB2 Hash Mathematics, Dynamic Resizing |
| **09** | Dynamic Rehashing & Load Factor Control | Phase I (W2) | Load factor $\ge 0.75$, power-of-2 resizing, bitwise masking |
| **10** | Protocol Grammar, Tokenizer & Interactive REPL | Phase I (W3) | EBNF Grammar, Quoted Tokenizer, Escapes, `bin/kvstore` CLI |
| **11** | Non-Blocking TCP Network Architecture | Phase II (W4–W5) | POSIX Sockets, `poll(2)` Multiplexing, `bin/kvstore_client` |
| **12** | Streaming Protocol Engine & ClientBuffer | Phase II (W5) | 1-Byte Chunk Fragmentation, Partial Reads, Pipelining |
| **13** | Asynchronous Signal Handling & Graceful Shutdown | Phase II (W6) | Self-Pipe Trick (`self_pipe[2]`), SIGINT/SIGTERM, Connection Drain |
| **14** | Write-Ahead Logging (WAL) & Binary Framing | Phase III (W7) | Binary Format, Network Byte Order, IEEE 802.3 CRC32, `fsync` |
| **15** | Startup Recovery Replay & Torn-Write Rejection | Phase III (W7–W8) | Sequential Replay Engine, CRC Validation, Broken Frame Detection |
| **16** | Crash Consistency Verification (`kill -9`) | Phase III (W8) | `scripts/crash_test.sh`, Sudden SIGKILL, 100% Data Preservation |
| **17** | Online Atomic Log Compaction Engine | Phase III (W9) | Live Snapshotting, Atomic `rename(2)` Swap, 98%+ Disk Reduction |
| **18** | Multi-Threaded Concurrency & Worker ThreadPool | Phase IV (W10) | Fixed-size ThreadPool, Work Queues, CondVar Synchronization |
| **19** | Fine-Grained Striped Reader-Writer Locking | Phase IV (W10) | 32 `pthread_rwlock_t` Stripes, Ascending Lock Ordering (0..31) |
| **20** | Key Expiry (TTL) Engine & Eviction Subsystem | Phase IV (W11) | Lazy Eviction on Access + Active Background Reaper Daemon |
| **21** | Empirical Benchmarking Methodology | Phase V (W12) | `bin/kvstore_bench` Architecture, Monotonic Timers, Percentiles |
| **22** | Empirical Throughput & Latency Results | Phase V (W12) | 7.22M OPS, 4.54x Speedup with Striped Locks, 98.7% p99 Drop |
| **23** | Full Test Suite Matrix & Regression Coverage | Phase V (W12) | 10 Test Binaries, 307 Assertions, 0 Failures Across All Tiers |
| **24** | Memory & Concurrency Audits (Valgrind & TSan) | Phase V (W12) | Valgrind: 0 bytes leaked in 0 blocks; TSan: 0 data races |
| **25** | Distinction Bar Compliance Checklist | Defense | Comprehensive 10-Item Distinction Bar Fulfillment Matrix |
| **26** | Individual Team Member Contributions | Defense | Engineering Breakdown: Rajnish (Core/Locks), Hardik (WAL), Om (Net) |
| **27** | Live Viva Demonstration Procedures | Defense | Exact Shell Commands for Live Evaluation & Viva Scripts |
| **28** | Conclusion & Technical Defense Open | Defense | Summary of Contributions & Faculty Technical Q&A Readiness |

---

## Slide-by-Slide Detailed Content

```carousel
### Slide 1: Cover Slide & Capstone Identity
- **Eyebrow**: `OS CAPSTONE · FINAL REVIEW · WEEKS 1–12 COMPLETE`
- **Title**: **KVStore**
- **Subtitle**: *An industrial-grade, crash-consistent, concurrent key-value store built in C11*
- **Approved Use Case**: Project 19 (KL University — 25CS2104E Operating Systems Capstone)
- **Repository**: `github.com/really-rajnish/KVStore`
- **Team**:
  - Rajnish Kumar Ranjan · `2500032608`
  - Hardik Gupta · `2500032616`
  - Om Amrit Das · `2500032631`
<!-- slide -->
### Slide 2: Project Abstract & Distinction Deliverables
- **Abstract**: High-performance persistent key-value store daemon developed from first principles in C11 with zero external dependencies.
- **Dual Storage Tier**: Combines an in-memory hash table ($O(1)$ lookup/insert) with an append-only binary Write-Ahead Log (WAL) guarded by IEEE 802.3 CRC32 checksums.
- **Distinction Bar Deliverables Completed**:
  - Crash Consistency (`kill -9`): 100% data preservation verified via automated tests.
  - Concurrency: 32 Striped Reader-Writer Locks achieving a **4.54x throughput speedup** and **98.7% p99 tail latency drop**.
  - Online Log Compaction: Atomic live snapshotting via POSIX `rename(2)` achieving **98%+ disk log reduction**.
  - Expiration Subsystem: Millisecond-precision TTL with dual-mode eviction (lazy on-access + active background reaper).
  - Strict OS Hygiene: **0 bytes leaked** under Valgrind; **0 data races** under ThreadSanitizer.
<!-- slide -->
### Slide 3: Chronological 12-Week Roadmap
- **Phase I: Core Engine (Weeks 1–3)**: Separate chaining hash table, DJB2 hashing, dynamic load-factor resizing, EBNF grammar, quoted tokenization, interactive REPL CLI.
- **Phase II: Networking (Weeks 4–6)**: Non-blocking POSIX sockets, streaming line protocol, `ClientBuffer` with 1-byte chunking & pipelining, Self-Pipe Trick async signal handling.
- **Phase III: Durability (Weeks 7–9)**: Binary WAL framing, IEEE 802.3 CRC32, startup recovery replay state machine, `kill -9` crash test harness, online atomic log compaction.
- **Phase IV: Concurrency (Weeks 10–12)**: ThreadPool, 32 striped RW-locks, TTL engine with active background reaper, empirical benchmarking harness, live demonstration script.
<!-- slide -->
### Slide 4: Problem Statement & Systems Engineering Dilemma
- **The Core Dilemma**: Memory Volatility vs. Disk Latency. Pure memory caches offer sub-millisecond latencies but suffer total loss of state upon crash. Disk databases prevent data loss but suffer severe I/O latency bottlenecks.
- **Four Fundamental OS Challenges**:
  1. *Crash Consistency (`kill -9`)*: Abrupt termination mid-write creates torn writes and corrupted indexes.
  2. *Lock Contention*: A global mutex serializes all CPU cores and kills scalability.
  3. *Signal Re-entrancy*: Calling non-reentrant functions (`malloc`, `printf`, sockets) inside signal handlers deadlocks daemons.
  4. *Descriptor & Memory Leaks*: Unbounded memory growth and descriptor leaks crash production servers.
<!-- slide -->
### Slide 5: Project Objectives, Scope & Constraints
- **Core Objectives**: High-speed in-memory indexing, binary WAL crash durability, scalable striped concurrency, and zero third-party dependencies.
- **In-Scope Deliverables**: Multi-threaded TCP daemon (`bin/kvstore_server`), interactive REPL (`bin/kvstore`), client utility (`bin/kvstore_client`), benchmark generator (`bin/kvstore_bench`), 10 test suites.
- **Scope Boundaries**: Single-node operating systems daemon (distributed consensus/Raft out of scope); assumes 64-bit virtual memory space; supports text and binary strings up to 64 KB.
<!-- slide -->
### Slide 6: Comprehensive 4-Tier System Architecture
```
[ Tier 1: Client Interfaces ]
   ├── Interactive REPL CLI (bin/kvstore)
   └── TCP Client Utility (bin/kvstore_client)
              │
[ Tier 2: Network & Signal Daemon ]
   ├── Non-Blocking Event Loop (poll(2))
   └── Async Signal Handler via Self-Pipe Trick (self_pipe[2])
              │
[ Tier 3: Protocol Framing & Concurrency Layer ]
   ├── Streaming Protocol Parser & ClientBuffer
   └── 32 Striped Reader-Writer Locks (pthread_rwlock_t)
              │
[ Tier 4: Storage, TTL & Durability Engine ]
   ├── In-Memory Hash Index (DJB2 + Dynamic Rehashing)
   ├── Active TTL Reaper Thread + Lazy Eviction
   ├── Write-Ahead Log (WAL) with IEEE CRC32 Checksums
   └── Online Atomic Compaction Engine (rename(2) swap)
```
<!-- slide -->
### Slide 7: End-to-End Query Processing Lifecycle
1. **Socket Read**: Client transmits `SET user:1 "Alice Smith" 5000` over TCP. `poll()` wakes worker thread.
2. **Buffer Accumulation**: `server.c` appends incoming bytes to dynamic `ClientBuffer`; extracts full line at CRLF delimiter.
3. **Parsing & Tokenizing**: `parser.c` tokenizes line, handles quoted strings and escapes, and validates argument arity.
4. **WAL Serialization**: `wal.c` serializes record, calculates IEEE 802.3 CRC32, and writes frame to disk log.
5. **Lock Acquisition**: Hash determines stripe index; thread acquires `pthread_rwlock_wrlock` for that stripe.
6. **Hash Table Mutation**: Separate chaining collision chain updated with new key, value, and `expire_at_ms`.
7. **Reply Dispatch**: `protocol.c` formats `OK\r\n` and flushes response back through socket to client.
<!-- slide -->
### Slide 8: In-Memory Index Design & DJB2 Hashing (Weeks 1 & 2)
- **Separate Chaining Architecture**: Bucket array with singly-linked `Node` chains storing dynamically allocated string keys and values.
- **DJB2 Hash Algorithm**: `hash = ((hash << 5) + hash) + c` — Daniel J. Bernstein's 33-multiplier hash providing uniform distribution and minimal clustering.
- **Why Separate Chaining over Open Addressing?**: Clean deletions without tombstone markers, fine-grained bucket locking, and predictable heap allocation.
- **Memory Discipline**: All keys and values duplicated with `strdup`; freed explicitly on deletion (`DEL`).
<!-- slide -->
### Slide 9: Dynamic Rehashing & Load Factor Control (Week 2)
- **Load Factor Threshold**: $\alpha = \frac{\text{size}}{\text{capacity}}$. When $\alpha \ge 0.75$, capacity automatically doubles ($16 \to 32 \to 64 \dots$).
- **Zero Node Duplication**: Existing `Node` pointers are spliced and re-linked into the newly allocated bucket array without duplicating string payloads.
- **Bitwise Modulo Optimization**: Capacities are powers of 2, allowing fast bitwise masking: `bucket = hash & (capacity - 1)`.
- **Amortized $O(1)$ Guarantee**: Rehashing cost amortizes to $O(1)$ per operation, preventing performance degradation.
<!-- slide -->
### Slide 10: Protocol Grammar, Tokenizer & REPL CLI (Week 3)
- **EBNF Grammar**: `line := wsp* command wsp* ( args )* wsp* newline?`
- **Supported Commands**: `SET`, `GET`, `DEL`, `COUNT`, `EXPIRE`, `TTL`, `COMPACT`, `EXIT`, `QUIT`.
- **Token Parsing**: Distinguishes bare tokens from double-quoted strings with escaped characters (`\"`, `\\`, `\n`).
- **Interactive REPL (`bin/kvstore`)**: Colored terminal prompt (`kvstore>`), syntax validation, and formatted replies.
- **Unit Testing**: `test_parser` (30 assertions) verifies tokenization, quotes, and edge cases.
<!-- slide -->
### Slide 11: Non-Blocking TCP Network Architecture (Weeks 4 & 5)
- **POSIX Sockets API**: Standard `AF_INET` stream sockets (`SOCK_STREAM`) with `fcntl(fd, F_SETFL, O_NONBLOCK)`.
- **I/O Multiplexing**: Single-threaded / multi-worker event loop using `poll(2)` to monitor listening socket, client descriptors, and self-pipe.
- **Ephemeral Port Binding**: Binds to port `0` for automated dynamic port allocation during regression test suites.
- **Client Utility (`bin/kvstore_client`)**: Supports both interactive terminal use and automated shell script pipelines.
<!-- slide -->
### Slide 12: Streaming Protocol Engine & ClientBuffer (Week 5)
- **TCP Stream Fragmentation**: TCP provides continuous byte streams; `recv()` may return partial commands or multiple combined commands.
- **Dynamic ClientBuffer**: Expanding byte buffer maintaining write head and unread read cursor per active socket.
- **1-Byte Chunking Resilience**: Verified via `test_protocol` (56 assertions) feeding commands 1 byte per `recv()` call without dropping frames.
- **Command Pipelining**: Back-to-back commands in a single socket buffer are unpacked and executed sequentially.
<!-- slide -->
### Slide 13: Asynchronous Signal Handling & Graceful Shutdown (Week 6)
- **The Self-Pipe Trick (`self_pipe[2]`)**:
  - Standard signal handlers cannot invoke `printf`, `malloc`, or socket calls safely.
  - Signal handler only writes 1 byte into `self_pipe[1]`, which is async-signal-safe.
  - `poll()` loop monitors `self_pipe[0]`, wakes up immediately, and triggers clean exit.
- **Connection Draining**: Closes listening socket, drains in-flight requests, flushes remaining socket buffers, and cleanly frees state.
<!-- slide -->
### Slide 14: Write-Ahead Logging (WAL) & Binary Framing (Week 7)
- **Binary Record Layout (Network Byte Order)**:
  `[Op: 1B] [KeyLen: 2B] [ValLen: 4B] [Key: N Bytes] [Val: M Bytes] [ExpireAt: 8B] [CRC32: 4B]`
- **IEEE 802.3 CRC32**: 32-bit polynomial protecting all preceding header and payload bytes.
- **Append-Only Logging**: Strict sequential append to `data/kvstore.wal`.
- **`fsync` Synchronization**: Configurable durability policies (`WAL_SYNC_ALWAYS` vs default buffered mode).
<!-- slide -->
### Slide 15: Startup Recovery Replay & Torn-Write Rejection (Weeks 7 & 8)
- **Startup Replay State Machine**: Traverses WAL sequentially, unpacks binary frames, verifies CRC32 checksums, and restores in-memory index.
- **Torn-Write Rejection**: When an abrupt crash occurs mid-write, the trailing record is truncated. Replay detects CRC mismatch, halts replay cleanly, and preserves all previously committed entries.
- **Corruption Injection Testing**: `test_wal` (20 assertions) validates rejection of artificial bit flips and truncated frames.
<!-- slide -->
### Slide 16: Crash Consistency Verification (`kill -9`) (Week 8)
- **Automated Durability Test (`scripts/crash_test.sh`)**:
  1. Spawns `bin/kvstore_server` on port 19876 with WAL enabled.
  2. Ingests 500 verified key-value mutations.
  3. Executes abrupt `kill -9 $SERVER_PID` mid-stream (no cleanup handlers executed).
  4. Restarts server daemon and queries all 500 keys.
- **Verification Result**: **100% data recovery** with zero corrupted records and zero descriptor leaks.
<!-- slide -->
### Slide 17: Online Atomic Log Compaction Engine (Week 9)
- **The Problem**: Append-only logs grow indefinitely. 100,000 updates to one key create 100,000 log entries.
- **Live Snapshot Swap**:
  1. Iterates live hash table; writes current live state to `data/kvstore.wal.compact.tmp`.
  2. Prunes deleted keys (tombstones) and expired keys.
  3. Forces `fsync()` and closes temporary file.
  4. Swaps file via atomic POSIX `rename(2)` swap over active WAL file.
- **Performance**: Achieves **98%+ disk log reduction** (13.9 KB bloated WAL $\to$ 280 bytes).
<!-- slide -->
### Slide 18: Multi-Threaded Concurrency & Worker ThreadPool (Week 10)
- **Fixed-Size ThreadPool**: Spawns worker threads at boot to avoid thread creation overhead.
- **Bounded Task Queue**: Synchronized using `pthread_mutex_t` and `pthread_cond_t` signaling.
- **Atomic Counters**: C11 `stdatomic.h` used for size tracking, eliminating counter contention.
- **ThreadSanitizer Clean**: Verified 0 data races under 16 concurrent threads (`make tsan`).
<!-- slide -->
### Slide 19: Fine-Grained Striped Reader-Writer Locking (Week 10)
- **32 Independent RW-Locks**: Hash space mapped to 32 stripes: `stripe = (djb2(key) % capacity) % 32`.
- **Parallel Scaling**: Readers on different keys run in parallel; writes only lock 1/32 of the key space.
- **Deadlock Prevention**: Full table operations lock stripes in strict ascending order ($0 \dots 31$).
- **Throughput Gain**: **4.54x throughput speedup** over coarse global mutex.
<!-- slide -->
### Slide 20: Key Expiry (TTL) Engine & Eviction Subsystem (Week 11)
- **Dual-Mode Eviction Architecture**:
  1. *Lazy Eviction on Access*: `GET` queries verify expiration timestamp. If expired, upgrades to write lock, purges node, and returns `(nil)`.
  2. *Active Background Reaper*: Dedicated thread wakes every 100ms, scans bucket slices, and frees unaccessed expired keys.
- **WAL Durability**: Expirations stored in WAL frames and restored on replay.
<!-- slide -->
### Slide 21: Empirical Benchmarking Methodology (Week 12)
- **Benchmark Tool (`bin/kvstore_bench`)**: Spawns concurrent POSIX threads executing operations against key space $K$.
- **High-Precision Timing**: POSIX `clock_gettime(CLOCK_MONOTONIC)` with sub-microsecond precision.
- **Latency Percentiles**: Latency samples sorted via `qsort` to measure exact p50, p90, p99, and max latencies.
- **Workloads Evaluated**: Read-Heavy Cache (95% R / 5% W), Balanced Store (50% R / 50% W), Write-Heavy (10% R / 90% W).
<!-- slide -->
### Slide 22: Empirical Throughput & Latency Results (Week 12)
- **Throughput Results (95% Read / 5% Write)**:
  - Striped RW-Locks: **7,221,684 OPS**
  - Global Mutex: **1,591,786 OPS**
  - Speedup: **4.54x Throughput Improvement**
- **Latency Percentiles (95% Read)**:
  - p50 (Median): Striped RW **0.1 µs** vs Global Mutex **0.4 µs** (4x faster)
  - p99 (Tail): Striped RW **1.0 µs** vs Global Mutex **76.0 µs** (**98.7% tail latency reduction**)
<!-- slide -->
### Slide 23: Full Test Suite Matrix & Regression Coverage (Week 12)
- `bin/test_hash_table`: 28 assertions PASS
- `bin/test_kvstore`: 18 assertions PASS
- `bin/test_parser`: 30 assertions PASS
- `bin/test_protocol`: 56 assertions PASS
- `bin/test_server_client`: 23 assertions PASS
- `bin/test_signals`: 10 assertions PASS
- `bin/test_wal`: 20 assertions PASS
- `bin/test_compaction`: 30 assertions PASS
- `bin/test_concurrency`: 56 assertions PASS
- `bin/test_ttl`: 36 assertions PASS
- **Total Assertions**: **307 / 307 PASS (0 Failures across 10 test suites)**.
<!-- slide -->
### Slide 24: Memory & Concurrency Audits (Valgrind & TSan) (Week 12)
- **Valgrind Memcheck (`make valgrind`)**:
  - `definitely lost: 0 bytes in 0 blocks`
  - `indirectly lost: 0 bytes in 0 blocks`
  - `0 errors from 0 contexts`
  - Descriptor tracking: 0 unclosed file or socket descriptors.
- **ThreadSanitizer (`make tsan`)**:
  - 16 concurrent threads executing 16,000 operations under `-fsanitize=thread`.
  - Zero data race warnings; zero lock order inversions.
<!-- slide -->
### Slide 25: Distinction Bar Compliance Checklist
- [x] O(1) In-Memory Index (DJB2 + Dynamic Rehashing)
- [x] Non-Blocking TCP Protocol Engine (1-byte chunking & pipelining)
- [x] Async-Signal-Safe Shutdown (Self-Pipe Trick `self_pipe[2]`)
- [x] Binary Write-Ahead Log with IEEE CRC32 Checksums
- [x] Crash Consistency surviving abrupt `kill -9` (`scripts/crash_test.sh`)
- [x] Online Atomic Log Compaction via `rename(2)` (98% reduction)
- [x] Striped Reader-Writer Locking (4.54x speedup, 98.7% p99 drop)
- [x] Sub-Millisecond Key Expiry (Lazy eviction + Active reaper daemon)
- [x] 100% Leak-Free Execution under Valgrind (0 bytes in 0 blocks)
- [x] Race Freedom under ThreadSanitizer (0 data races)
<!-- slide -->
### Slide 26: Individual Team Member Contributions
- **Rajnish Kumar Ranjan (`2500032608`)**: Core Hash Table, DJB2 hashing, 32-Stripe RW-locking architecture, Compaction engine, Makefile automation.
- **Hardik Gupta (`2500032616`)**: Write-Ahead Log binary serialization, IEEE CRC32 checksums, crash recovery replay engine, `kill -9` crash test harness.
- **Om Amrit Das (`2500032631`)**: Non-blocking TCP server daemon, dynamic `ClientBuffer` streaming protocol, Self-Pipe Trick signal handling, REPL CLI.
<!-- slide -->
### Slide 27: Live Viva Demonstration Procedures
1. `make clean && make all`: Compiles REPL, server, client, benchmark, and test binaries.
2. `bin/kvstore`: Interactive REPL demonstration (`SET`, `GET`, `DEL`, `COUNT`, `EXPIRE`, `TTL`).
3. `make test`: Executes all 10 test suites (307 assertions) in under 2 seconds.
4. `make valgrind`: Proves 0 memory leaks across test binaries.
5. `make tsan`: Verifies race freedom under multi-threaded soak testing.
6. `make crash-test`: Demonstrates `kill -9` kill followed by zero-loss database recovery.
7. `make bench`: Executes empirical concurrency benchmark.
8. `make demo`: Automated end-to-end presentation script walking through all features.
<!-- slide -->
### Slide 28: Conclusion & Technical Defense Open
- **Key Takeaways**: Industrial-grade systems engineering achieved with pure C11 and POSIX primitives; zero external libraries; proven crash consistency and multi-core scalability.
- **Floor Open for Defense**: Ready for evaluator questions on POSIX signals, concurrency, durability math, and live code inspection.
```

---

## Word-for-Word Final Defense Speaking Script

### Speaker 1: Rajnish Kumar Ranjan (`2500032608`)
**Presenting Slides: 1–5, 8–9, 17, 19, 25, 28**

> *"Respected evaluators and faculty members, good morning. Our team is proud to present the complete final capstone defense for **KVStore**, an industrial-grade, crash-consistent, concurrent in-memory key-value store daemon written from scratch in C11 for Project 19 of our Operating Systems Capstone.*
>
> *(Slides 2–5)* *Over the 12-week course, we engineered this system from first principles using pure POSIX primitives without any third-party dependencies. We set out to solve the classical systems engineering trade-off: memory stores achieve sub-millisecond speeds but suffer total data loss on crashes; disk databases preserve state but suffer massive I/O latency bottlenecks. Furthermore, existing industry tools like Redis rely on millions of lines of external code. Our goal was to achieve complete crash durability, fine-grained multi-core concurrency, asynchronous signal safety, and strict memory discipline.*
>
> *(Slides 8–9)* *In Phase I, I engineered the core in-memory index using a separate chaining hash table with Daniel J. Bernstein's DJB2 hash algorithm. To guarantee $O(1)$ operations under heavy insertions, I implemented dynamic power-of-2 bucket expansion. When the load factor exceeds 0.75, capacity doubles from 16 to 32, 64, and beyond. Rather than re-allocating nodes, existing linked nodes are directly spliced and re-linked into the new bucket array, preserving amortized $O(1)$ bounds.*
>
> *(Slide 17)* *In Phase III, I built our online atomic log compaction engine. Because append-only logs grow linearly, compaction creates a live snapshot in a temporary file, prunes tombstones and expired keys, and swaps the file into place using the POSIX `rename(2)` system call. This achieves a 98% reduction in disk usage while keeping the daemon online.*
>
> *(Slide 19)* *In Phase IV, I tackled multi-core lock contention. A global mutex creates a single-core bottleneck. I implemented 32 Striped Reader-Writer Locks using `pthread_rwlock_t`. Key hashes map to stripes via `(hash % capacity) % 32`. To prevent deadlocks during full-table operations like resizing or compaction, locks are strictly acquired in ascending index order from 0 to 31. This delivered a 4.54x throughput speedup over a global mutex.*
>
> *(Slide 25 & 28)* *We have fulfilled 100% of the syllabus and distinction bar criteria. I now hand over to Hardik Gupta to present our durability and crash consistency architecture."*

---

### Speaker 2: Hardik Gupta (`2500032616`)
**Presenting Slides: 14–16, 21–22, 24, 26**

> *(Slide 14)* *"Thank you, Rajnish. In Phase III, I led the durability and crash consistency architecture. To guarantee persistence without sacrificing performance, I designed a binary Write-Ahead Log in network byte order. Each mutation record is framed with an opcode, key length, value length, key and value payloads, an expiration timestamp, and an IEEE 802.3 32-bit CRC checksum computed across all fields.*
>
> *(Slide 15)* *During startup, the recovery engine parses records sequentially and recalculates the CRC32 checksum. This protects against the 'torn-write' phenomenon—where a process crashes or power drops mid-write, leaving truncated bytes at the tail of the log. If a CRC mismatch is detected, replay halts cleanly at the last valid record, rejects the corrupted partial write, and restores 100% of the committed data.*
>
> *(Slide 16)* *In Week 8, I automated our ungraceful termination testing in `scripts/crash_test.sh`. The harness boots the server, inserts 500 keys, issues an abrupt `kill -9` (`SIGKILL`) directly to the daemon process—preventing any cleanup handlers from running—and then reboots the server. Our replay engine restored 100% of all 500 keys with zero corrupted state and zero descriptor leaks.*
>
> *(Slides 21–22)* *In Week 12, I built our empirical benchmarking tool `bin/kvstore_bench`. Using POSIX monotonic timers, we measured throughput and exact latency percentiles across multi-threaded workloads. Under a 95% read workload, our 32-striped reader-writer locks achieved 7.22 Million OPS compared to 1.59 Million OPS for a global mutex—delivering a 4.54x speedup and reducing p99 tail latency from 76 microseconds down to 1.0 microsecond (a 98.7% drop).*
>
> *(Slide 24)* *Finally, our memory and race audits prove that our implementation is industrial-grade: Valgrind reports 0 bytes leaked in 0 blocks across all 10 binaries, and ThreadSanitizer reports 0 data races under 16 concurrent threads. I now hand over to Om Amrit Das to present our networking and signal handling subsystems."*

---

### Speaker 3: Om Amrit Das (`2500032631`)
**Presenting Slides: 6–7, 10–13, 18, 20, 23, 27**

> *(Slides 6–7)* *"Thank you, Hardik. In Phase II, I engineered our non-blocking networking layer, protocol framing, and signal handling.*
>
> *(Slides 10–12)* *Our client interface uses an ASCII line-based grammar with support for bare tokens and quoted strings with escape sequences. In TCP networking, a common flaw in naive servers is assuming that each `recv()` call returns one complete command. Because TCP is a continuous byte stream, packets can arrive in 1-byte fragments or multiple combined commands. I built a dynamic `ClientBuffer` that accumulates fragments across non-blocking socket reads until a CRLF delimiter is reached, and supports request pipelining without dropping frames. Our protocol tests pass 100% even under simulated 1-byte chunked streaming.*
>
> *(Slide 13)* *In Week 6, I tackled signal safety. In POSIX systems, signal handlers for `SIGINT` or `SIGTERM` cannot safely invoke non-reentrant functions like `malloc()`, `printf()`, or socket calls. I implemented the **Self-Pipe Trick**: our signal handler only writes 1 byte to a non-blocking internal pipe (`self_pipe[1]`). The server's main `poll()` loop monitors `self_pipe[0]`, immediately detects the wake-up, stops accepting new connections, drains in-flight client requests, and shuts down cleanly.*
>
> *(Slides 18 & 20)* *In Phase IV, I built our worker ThreadPool with condition variable signaling, and our millisecond-precision Key Expiry (TTL) subsystem. The TTL engine operates in dual mode: lazy eviction checks key timestamps on `GET` queries, while an active background reaper thread wakes every 100 milliseconds to scan buckets and purge abandoned keys.*
>
> *(Slides 23 & 27)* *Our entire project includes 10 test binaries with 307 assertions and zero failures. We are now ready for our live viva demonstration. We will demonstrate our interactive REPL CLI `bin/kvstore`, execute `make test` across all 307 assertions, run `make valgrind` to prove zero memory leaks, and run `make crash-test` to demonstrate live kill -9 crash consistency."*

---

## Live Demonstration Script for Viva

Run these commands live in the terminal during the evaluation:

```bash
# 1. Clean and compile all binaries
make clean
make all

# 2. Run the interactive REPL CLI (Demonstrating Phase I)
bin/kvstore
# Try these commands in the prompt:
# SET name "KVStore Distinction Prototype" 15000
# GET name
# COUNT
# TTL name
# EXIT

# 3. Execute all 10 Test Suites (307 assertions, 0 failures)
make test

# 4. Demonstrate Memory Safety (0 bytes leaked in 0 blocks)
make valgrind

# 5. Demonstrate Race Freedom under ThreadSanitizer
make tsan

# 6. Demonstrate Crash Consistency against abrupt kill -9
make crash-test

# 7. Execute Empirical Concurrency Benchmark (4.54x Speedup)
make bench

# 8. Run the Full Viva Demonstration Automation
make demo
```

---

## Faculty Viva Questions & Model Answers

### Q1: What is the Self-Pipe Trick and why is it necessary in a production daemon?
**Answer**:
> *"In POSIX systems, asynchronous signal handlers (such as for `SIGINT` or `SIGTERM`) must be strictly async-signal-safe. Standard library functions like `printf()`, `malloc()`, or socket functions are non-reentrant and can deadlock if called inside a signal handler. The **Self-Pipe Trick** creates a pipe (`self_pipe[2]`) in non-blocking mode. The signal handler only writes a single byte into `self_pipe[1]`, which is async-signal-safe. The server's main `poll()` loop monitors `self_pipe[0]`, immediately detects the wake-up, exits gracefully, drains active connections, and shuts down cleanly."*

### Q2: Why separate chaining with DJB2 over open addressing with linear probing?
**Answer**:
> *"Separate chaining handles dynamic deletions (`DEL`) and fine-grained locking much better than open addressing with linear probing. In open addressing, deletions require tombstone markers that degrade lookup performance, and locking requires complex bucket-range locking. In separate chaining, each stripe RW-lock protects a bucket's entire collision chain independently, eliminating cascading lock contention."*

### Q3: How does the system guarantee crash consistency against sudden `kill -9`?
**Answer**:
> *"When a process is killed with `SIGKILL` (`kill -9`), no application-level exit handlers or cleanup code can run. We achieve crash consistency through Write-Ahead Logging (WAL) with binary record framing and IEEE 802.3 CRC32 checksums. Before in-memory mutations are finalized, records are appended to disk and flushed. On recovery, the replay engine reads records sequentially. If a crash occurred mid-write (a torn write), the checksum verification fails, causing the recovery engine to stop at the last valid record, discard the partial bytes, and safely restore 100% of the committed data."*

### Q4: How do 32 Striped Reader-Writer Locks prevent deadlocks during full-table operations?
**Answer**:
> *"Single-key queries acquire only the lock corresponding to their hash stripe: `stripe = (hash % capacity) % 32`. However, full-table operations (such as dynamic table resizing or log compaction) must acquire write locks across all 32 stripes. To prevent deadlock (circular wait), we enforce a strict **canonical ascending lock acquisition order**: locks are acquired strictly from stripe 0 to stripe 31, and released in reverse order. Because all threads adhere to the exact same order, circular waits are mathematically impossible."*

### Q5: How does online log compaction work without taking the database offline?
**Answer**:
> *"Online compaction creates a live snapshot without freezing client traffic. It iterates through the live hash table and writes current valid keys to a temporary file (`data/kvstore.wal.compact.tmp`), omitting deleted keys and expired TTL keys. Once written, it calls `fsync()` to ensure disk persistence, closes the file, and performs an atomic POSIX `rename(2)` swap over the active WAL file. In the Linux kernel, `rename(2)` is atomic, ensuring that any crash during compaction leaves either the old log or the new log intact, never a corrupt hybrid."*
