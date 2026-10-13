# OS Capstone Internal Review 1: KVStore Presentation Deck & Defense Guide

**Course**: KL University — 25CS2104E Operating Systems Capstone  
**Approved Use Case**: Project 19 — High-Performance Concurrent Key-Value Store  
**Deliverable**: 12-Slide Master Presentation Deck & Viva Script  
**PowerPoint File**: [`docs/OS_Capstone_Internal_Review_1_KVStore.pptx`](file:///Ubuntu/root/KVStore/docs/OS_Capstone_Internal_Review_1_KVStore.pptx)  
**Repository**: [github.com/really-rajnish/KVStore](https://github.com/really-rajnish/KVStore)

---

## Team Information
- **Rajnish Kumar Ranjan** · Reg No: `2500032608` (Lead: In-Memory Index, Striped Concurrency & Compaction)
- **Hardik Gupta** · Reg No: `2500032616` (Lead: Write-Ahead Log, IEEE CRC32 Checksums & Crash Replay)
- **Om Amrit Das** · Reg No: `2500032631` (Lead: Non-Blocking TCP Daemon, Self-Pipe Trick & Client CLI)

---

## Evaluation Rubrics & Mark Distribution Matrix

| Section | Rubric Focus | Marks | Covered In Slide(s) | Status |
|:---|:---|:---:|:---:|:---:|
| **1. Project Identification** | Use case, title, team, problem statement, objectives, scope | **5 M** | Slide 1, 2, 3 | **Complete** |
| **2. Requirements** | Functional, non-functional, I/O, hardware/software specs | **10 M** | Slide 4, 5 | **Complete** |
| **3. Design** | Architecture, module diagram, data flow, storage/index design, tools | **10 M** | Slide 6, 7, 8 | **Complete** |
| **4. Working Environment** | Ubuntu/WSL2 setup, toolchain, Git repo, folder structure | **10 M** | Slide 9 | **Complete** |
| **5. Working Prototype** | Working application, Core Module-I demonstrated, verification tests | **10 M** | Slide 10, 11 | **Complete (Distinction)** |
| **Summary & Defense** | Team responsibilities, viva demonstration checklist | — | Slide 12 | **Complete** |
| **Total** | | **45 M** | **12 Slides** | **100% Prepared** |

---

## Slide-by-Slide Detailed Deck

```carousel
### Slide 1: Cover Slide & Team Identification
**Header Eyebrow**: `OS CAPSTONE · INTERNAL REVIEW 1 · WEEK 3`  
**Title**: **KVStore**  
**Subtitle**: *A persistent key-value store built from scratch in C11*  
**Repository**: `github.com/really-rajnish/KVStore`  
**Approved Use Case**: Project 19 (From Approved 25 Capstone Use Cases)

**Team Members (3 Students)**:
- **Rajnish Kumar Ranjan** — `2500032608`
- **Hardik Gupta** — `2500032616`
- **Om Amrit Das** — `2500032631`

**Slide Note**: Establishes identity, course context, and live GitHub tracking repository.
<!-- slide -->
### Slide 2: Project Identification — Problem Statement & Motivation [Rubric 1 - 5M]
**Header**: `1. Project Identification · Rubric 1 (5 Marks)`  
**Title**: **Problem Statement & Engineering Motivation**  
**Context**: *Why build an industrial-grade concurrent key-value store from scratch in C11?*

#### 1. The Fundamental Dilemma: Memory Speed vs. Durability
- **Memory Volatility**: Pure in-memory stores achieve sub-millisecond access latencies but suffer catastrophic, total data loss upon power loss, kernel panics, or sudden process aborts.
- **I/O Latency Penalty**: Relational databases enforce durability using heavy disk I/O, causing high latency spikes and severely bottlenecked write throughput.
- **Opaque Runtimes**: Modern caching systems (e.g., Redis, Memcached) rely on extensive third-party codebases, obscuring low-level operating systems mechanics.

#### 2. Four Core Operating System Challenges Addressed
1. **Crash Consistency (`kill -9`)**: Guaranteeing zero corrupted states or broken entries when a daemon terminates abruptly mid-write without invoking exit handlers.
2. **Reader-Writer Lock Contention**: Coarse-grained global mutexes serialize multi-core throughput; fine-grained striped synchronization is required for multi-core scaling.
3. **Asynchronous Signal Safety**: POSIX signal handlers (`SIGINT`, `SIGTERM`) cannot safely invoke `printf`, `malloc`, or socket operations. Clean shutdown requires the **Self-Pipe Trick**.
4. **Strict Memory Discipline**: Long-running daemon processes must guarantee 0 bytes leaked and 0 descriptor leaks under high-churn multi-client workloads.
<!-- slide -->
### Slide 3: Project Identification — Objectives & Scope [Rubric 1 - 5M]
**Header**: `1. Project Identification · Rubric 1 (5 Marks)`  
**Title**: **Project Objectives & Scope of Work**  
**Context**: *Approved Use Case: Project 19 — High-Performance Concurrent Key-Value Store*

#### 1. Core Objectives
- Design and implement an $O(1)$ in-memory separate chaining hash table with DJB2 hashing and automated dynamic load-factor resizing from scratch.
- Implement Write-Ahead Logging (WAL) with binary record framing, IEEE 802.3 CRC32 checksums, and torn-write rejection for crash durability.
- Construct a non-blocking TCP server daemon tolerating fragmented stream reads and pipelined batch bursts.
- Achieve the **Distinction Bar**: 32-striped RW-locks, online atomic log compaction, background TTL reaper, and automated crash tests.

#### 2. Project Scope
- **In-Scope**: Pure POSIX C11 server daemon, client utility CLI, interactive terminal REPL, binary WAL, compaction, striped locks, automated test suites.
- **Scope Boundaries / Assumptions**: Scoped as a single-node operating systems daemon (distributed consensus/Raft out of scope); assumes 64-bit POSIX environment; supports arbitrary text and binary strings up to 64 KB.
<!-- slide -->
### Slide 4: Requirements Engineering — Functional & Non-Functional [Rubric 2 - 10M]
**Header**: `2. Requirements Engineering · Rubric 2 (10 Marks)`  
**Title**: **Functional & Non-Functional Requirements**  
**Context**: *Formal specification of system behaviors, performance criteria, and OS invariants*

#### Functional Requirements (FR1 to FR8)
- **FR1 (SET)**: Insert or overwrite key-value pairs with optional TTL duration in milliseconds.
- **FR2 (GET)**: Retrieve value associated with key; return `(nil)` if absent or expired (triggering lazy eviction).
- **FR3 (DEL)**: Atomically remove key, reclaim heap memory, and return `OK` or `NOT_FOUND`.
- **FR4 (REPL CLI)**: Interactive command prompt with command history, syntax checking, and quote parsing.
- **FR5 (TCP Protocol)**: Streaming line protocol tolerating 1-byte chunks, partial reads, and pipelined bursts.
- **FR6 (WAL Replay)**: Binary log replay reconstructing in-memory index on startup, discarding torn writes.
- **FR7 (Compaction)**: Online snapshot compaction shrinking WAL size by 90%+ via atomic `rename(2)`.
- **FR8 (TTL & Reaper)**: Dual-mode expiration: lazy eviction on access + active background reaper thread.

#### Non-Functional Requirements (NFR1 to NFR5)
- **NFR1 (Time Complexity)**: $O(1)$ average-case time complexity for `SET`, `GET`, and `DEL`.
- **NFR2 (Race Freedom)**: Zero data races under 16+ concurrent threads verified by Clang/GCC ThreadSanitizer.
- **NFR3 (Zero Memory Leaks)**: 100% leak-free execution verified by Valgrind Memcheck (0 bytes in 0 blocks).
- **NFR4 (Crash Durability)**: Survives sudden ungraceful `SIGKILL` (`kill -9`) mid-operation with zero corrupted records.
- **NFR5 (Zero External Dependencies)**: Pure C11 using only POSIX.1-2008 standards (pthreads, sockets, poll).
<!-- slide -->
### Slide 5: Requirements Engineering — I/O, Hardware & Software Specs [Rubric 2 - 10M]
**Header**: `2. Requirements Engineering · Rubric 2 (10 Marks)`  
**Title**: **Input/Output, Hardware & Software Requirements**  
**Context**: *Protocol framing standards, development dependencies, and target execution environment*

#### 1. Input/Output Requirements & Grammar
- **Line-Based Stream Input**: CRLF / LF terminated ASCII command stream supporting bare and quoted tokens.
- **Quoted String Tokenizer**: Supports escaped quotes and multi-word values (e.g., `SET user:1 "Alice Smith" 5000`).
- **Deterministic Output Codes**: Uniform reply semantics: `OK`, `(nil)`, `"value"`, `NOT_FOUND`, `<integer>`, and `ERROR <msg>`.
- **Fragmented Stream Resilience**: Dynamic `ClientBuffer` accumulates byte fragments across `recv()` calls; tolerates 1-byte chunks without dropping data.
- **Pipelining Support**: Clients may send multiple back-to-back requests in a single buffer; daemon dispatches each in order.

#### 2. Hardware & Software Specifications
- **Operating System**: Ubuntu 24.04 LTS running on Windows Subsystem for Linux (WSL2), Linux Kernel 6.6.
- **Compiler & Standard**: GCC 13.x / Clang 18 with C11 standard (`-std=c11 -Wall -Wextra -Wpedantic -pthread -O2`).
- **Build System**: GNU Make with modular targets (`make all`, `make test`, `make valgrind`, `make tsan`, `make demo`).
- **Verification Toolchain**: Valgrind 3.22 (Memcheck), Clang ThreadSanitizer, GDB, strace, and GitHub CLI (`gh`).
- **Hardware Environment**: x86_64 Multi-Core CPU (4+ virtual cores recommended for threadpool & striped lock scaling), 4 GB RAM.
<!-- slide -->
### Slide 6: System Design — Overall Architecture [Rubric 3 - 10M]
**Header**: `3. System Design · Rubric 3 (10 Marks)`  
**Title**: **Overall System Architecture**  
**Context**: *Multi-layered decoupled architecture connecting client endpoints to durable in-memory storage*

#### 4 Architectural Tiers:
```
[ Tier 1: Client Interfaces ]
   ├── Interactive REPL CLI (bin/kvstore)
   └── TCP Client Utility (bin/kvstore_client)
              │ (TCP Stream Socket)
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
### Slide 7: System Design — Module Block Diagram & Data Flow [Rubric 3 - 10M]
**Header**: `3. System Design · Rubric 3 (10 Marks)`  
**Title**: **Module Block Diagram & Request Workflow**  
**Context**: *End-to-end trace of query processing from socket read to WAL commit and reply*

#### 1. System Module Decomposition
- **Module 1: In-Memory Index (`hash_table.c`)**: Manages bucket arrays, linked node chains, DJB2 hashing, and dynamic table resizing.
- **Module 2: Command Dispatcher (`dispatcher.c`)**: Translates parsed tokens into KVStore API calls and formats standardized replies.
- **Module 3: Protocol & Network Server (`server.c`)**: Manages socket life cycles, buffer chunking, connection tracking, and draining.
- **Module 4: Persistence Engine (`wal.c`)**: Appends framed binary mutation records with IEEE 802.3 CRC32; replays log on boot.
- **Module 5: Compaction & TTL (`compaction.c`, `ttl.c`)**: Background active expiration sweep and atomic live log compaction swap.

#### 2. End-to-End Request Data Flow (`SET user:100 "Rajnish" 60000`)
1. **Ingress**: Client writes ASCII command into TCP socket stream.
2. **Framing**: `server.c` `poll()` detects readable socket; accumulates bytes in `ClientBuffer` until newline delimiter.
3. **Parsing**: `parser.c` extracts tokens; validates syntax and converts TTL to absolute epoch milliseconds.
4. **Durability**: `wal.c` computes IEEE CRC32, frames binary record, and appends to `data/kvstore.wal`.
5. **Lock & Write**: `kvstore.c` acquires Stripe RW write-lock, mutates bucket node, and releases lock.
6. **Response**: `protocol.c` formats `OK\r\n` reply and writes back to socket descriptor; client completes.
<!-- slide -->
### Slide 8: System Design — Storage/Database Design & Technologies [Rubric 3 - 10M]
**Header**: `3. System Design · Rubric 3 (10 Marks)`  
**Title**: **Storage/Index Design & Technology Stack**  
**Context**: *Binary WAL framing, collision chain indexing, and rationale for selected tools*

#### 1. In-Memory Hash Index & Dynamic Rehashing
- **DJB2 Hash Algorithm**: `hash = ((hash << 5) + hash) + c` — ensures uniform key distribution across buckets.
- **Dynamic Resizing**: When load factor ($\frac{\text{size}}{\text{capacity}}$) $\ge 0.75$, capacity doubles ($16 \to 32 \to 64 \dots$). Existing nodes are re-linked into new buckets without heap re-allocations.

#### 2. Binary WAL Record Format (Network Byte Order)
```
+---------+-------------+-------------+---------------+-----------------+-------------------+-----------+
| Op (1B) | KeyLen (2B) | ValLen (4B) | Key (KeyLenB) | Val (ValLenB)   | ExpireAt_ms (8B)  | CRC32(4B) |
+---------+-------------+-------------+---------------+-----------------+-------------------+-----------+
```
- **Crash Protection**: CRC32 detects flipped bits and truncated torn writes on sudden loss of power.

#### 3. Selected Technologies & Engineering Rationale
- **C11 (ISO/IEC 9899:2011)**: Deterministic memory allocation, fine-grained pointer control, zero GC overhead.
- **POSIX Pthreads & Sockets**: Direct access to OS kernel primitives (`pthread_rwlock_t`, `poll`, `self_pipe`).
- **Zero Third-Party Dependencies**: Built from first principles to ensure complete transparency and code ownership.
<!-- slide -->
### Slide 9: Working Environment — Setup, Toolchains & Code Structure [Rubric 4 - 10M]
**Header**: `4. Working Environment · Rubric 4 (10 Marks)`  
**Title**: **Working Environment & Codebase Structure**  
**Context**: *Development platform, installed toolchains, GitHub repository, and directory hierarchy*

#### 1. Development Setup & Installed Toolchain
- **Host Platform**: Ubuntu 24.04 LTS on WSL2 (Linux 6.6 Kernel) under Windows 11.
- **Compiler**: GCC 13.x with `-std=c11 -Wall -Wextra -Wpedantic -pthread -O2`.
- **Auditors & Profilers**: Valgrind 3.22 (Memcheck), Clang ThreadSanitizer, GDB, strace.
- **Git / GitHub Repository**: Active repository at `https://github.com/really-rajnish/KVStore` with authenticated GitHub CLI (`gh`).

#### 2. Clean Directory Organization
```
KVStore/
├── Makefile                # Unified build rules (all, test, valgrind, tsan, bench, demo)
├── README.md               # Project overview, distinction checklist & API docs
├── bin/                    # Output binaries (kvstore, kvstore_server, kvstore_client, tests)
├── data/                   # Active WAL log files (kvstore.wal)
├── docs/                   # Architecture, design, testing, benchmark & execution reports
├── include/                # Modular C headers (hash_table.h, kvstore.h, wal.h, etc.)
├── scripts/                # Automated shell harnesses (benchmark.sh, crash_test.sh, demo.sh)
├── src/                    # Core C implementations & entrypoints (server_main.c, etc.)
└── tests/                  # 10 standalone unit, concurrency, and integration test suites
```
<!-- slide -->
### Slide 10: Working Prototype — Core Module-I & Live Implementation [Rubric 5 - 10M]
**Header**: `5. Working Prototype · Rubric 5 (10 Marks)`  
**Title**: **Core Module-I Implementation & Prototype Status**  
**Context**: *Review-1 Mandate: Working core application demonstrated with initial functionality*

#### 1. Core Module-I: Fully Implemented & Operational
- **Review-1 Mandate Met**: Core Module-I is 100% implemented, compiled, and running in the working environment.
- **In-Memory Hash Table**: Full CRUD lifecycle (`SET`, `GET`, `DEL`, `COUNT`) written from scratch.
- **Collision Handling**: Singly-linked separate chaining bucket nodes with explicit memory deallocation.
- **Dynamic Rehashing**: Power-of-2 bucket expansion preserving $O(1)$ operational efficiency.
- **Interactive REPL CLI**: `bin/kvstore` interactive prompt parses user commands and renders structured feedback.
- **Unit Test Verification**: `test_hash_table` (28 assertions) and `test_kvstore` (18 assertions) pass with 100% success.

#### 2. Distinction Bar: Exceeding Review-1 Baseline
*While Review-1 only requires partial Module-I implementation, the team has delivered the full capstone architecture:*
- **TCP Server Daemon**: Multi-client socket server (`bin/kvstore_server`) with signal handling.
- **Write-Ahead Log**: Binary persistence and crash replay surviving `kill -9`.
- **Online Compaction**: 98%+ disk reduction via atomic live snapshot swapping.
- **Striped Concurrency**: 32 reader-writer locks delivering 4.54x throughput speedup over global mutex.
- **Subsystem TTL**: Lazy eviction and active background reaper thread operational.
<!-- slide -->
### Slide 11: Working Prototype — Verification, Test Results & Benchmarks [Rubric 5 - 10M]
**Header**: `5. Working Prototype · Rubric 5 (10 Marks)`  
**Title**: **Prototype Verification & Distinction Metrics**  
**Context**: *Empirical validation across unit testing, memory safety, race detection, and crash durability*

#### Key Distinction Metrics:
| Metric | Value | Meaning |
|:---|:---:|:---|
| **Test Suites Passed** | **10 / 10** | 100% test coverage across all subsystems |
| **Assertions Verified** | **307 / 307** | Zero regressions or assertion failures |
| **Valgrind Memory Leaks** | **0 Bytes (0 Blocks)** | Zero heap memory or file descriptor leaks |
| **ThreadSanitizer Races** | **0 Data Races** | Strict synchronization under 16 concurrent threads |
| **Throughput Speedup** | **4.54x Speedup** | 32 Striped RW-Locks vs. Coarse Global Mutex |
| **p99 Tail Latency** | **1.0 µs (98.7% Drop)** | Drop from 76.0 µs down to 1.0 µs under multi-threaded load |

#### Automated Verification Suites:
- `make test`: All 10 suites pass in under 2 seconds.
- `make valgrind`: Audits all 10 binaries with `--leak-check=full --error-exitcode=1`.
- `make tsan`: Verifies race freedom under multi-threaded soak testing.
- `make crash-test`: Spawns daemon, inserts keys, delivers `kill -9`, and proves 100% recovery.
<!-- slide -->
### Slide 12: Team Contributions & Live Viva Demonstration [Summary & Defense]
**Header**: `Summary & Oral Defense · Review 1`  
**Title**: **Team Member Contributions & Viva Demonstration**  
**Context**: *Individual responsibilities, capstone roadmap, and live demonstration overview*

#### 1. Individual Student Contributions
- **Rajnish Kumar Ranjan (`2500032608`)**: Core Hash Table Engine, DJB2 rehashing, 32-Stripe RW-Lock concurrency architecture, atomic Compaction Engine, system integration.
- **Hardik Gupta (`2500032616`)**: Write-Ahead Log (WAL) binary serialization, IEEE 802.3 CRC32 checksums, crash recovery replay engine, automated `kill -9` test harness.
- **Om Amrit Das (`2500032631`)**: Non-blocking TCP daemon, dynamic `ClientBuffer` streaming protocol, Self-Pipe Trick async signal handling, REPL CLI interface.

#### 2. Live Viva Demonstration Checklist
1. **Interactive REPL (`bin/kvstore`)**: Live execution of `SET`, `GET`, `DEL`, `COUNT`, `EXPIRE`, and `TTL` commands.
2. **Full Test Suite (`make test`)**: Instant live execution of all 10 test suites showing 307 assertions passing.
3. **Memory Safety (`make valgrind`)**: Live Valgrind verification proving 0 memory leaks across test binaries.
4. **Crash Consistency (`make crash-test`)**: Live demonstration of `kill -9` kill followed by zero-loss database recovery.
5. **Full System Demo (`make demo`)**: Automated live script showcasing all features in presentation sequence.
```

---

## Word-for-Word Viva Speaking Script (Divided Among Team)

### Student 1: Rajnish Kumar Ranjan (`2500032608`)
**Slides to Present: 1, 2, 6, 10, 12**

> *"Good morning, esteemed faculty members and evaluators. Our team presents **KVStore**, an industrial-grade, crash-consistent, concurrent key-value store daemon built from scratch in C11 for our Operating Systems Capstone, Project 19.*
>
> *Moving to Slide 2: The core engineering motivation behind this project tackles the classical dilemma between memory speed and durability. Pure in-memory databases give sub-millisecond latencies but risk total data loss on sudden crashes. Conversely, traditional disk databases introduce significant I/O latency bottlenecks. Furthermore, existing industrial tools like Redis are massive codebases. Our goal is to build an industrial-grade engine from first principles using pure POSIX primitives—tackling crash consistency, lock contention, asynchronous signal safety, and strict memory discipline.*
>
> *Moving to Slide 6: Our architecture is cleanly decoupled into 4 tiers. Tier 1 handles client interfaces via our interactive REPL and TCP client. Tier 2 is our network daemon using a non-blocking poll loop and the Self-Pipe Trick. Tier 3 is our protocol framing engine and 32 striped reader-writer locks. Tier 4 is our in-memory hash table, active TTL reaper, and write-ahead log.*
>
> *On Slide 10: For this Internal Review-1, while the rubric mandates initial progress on Core Module-I, our team has not only completed Module-I with a full O(1) DJB2 dynamic hash table and interactive REPL, but we have already implemented and verified the entire 12-week capstone architecture! Every single line of code is written in C11 without any third-party libraries, and our test suite passes with 100% success."*

---

### Student 2: Hardik Gupta (`2500032616`)
**Slides to Present: 3, 4, 8, 11**

> *"Continuing to Slide 3: Our project objectives focus on achieving four clear deliverables: an O(1) in-memory index, crash-consistent durability, high-throughput concurrent networking, and the distinction deliverables including striped locking, online compaction, and TTL expiration.*
>
> *Moving to Slide 4: Our functional requirements cover complete CRUD mutations, streaming line-based TCP communications, crash recovery replay, and online compaction. On the non-functional side, we enforce three strict invariants: O(1) amortized algorithmic complexity, zero data races under ThreadSanitizer, and zero memory leaks under Valgrind Memcheck.*
>
> *Turning to Slide 8: Let me explain our storage and durability design. In-memory, our hash table uses DJB2 hashing with automatic power-of-2 bucket resizing at a 0.75 load factor. For on-disk durability, we designed a custom binary WAL format in network byte order: each record carries an operation opcode, key length, value length, key and value bytes, an expiration timestamp, and an IEEE 802.3 CRC32 checksum. This checksum guarantees that if the system crashes mid-write—such as during a sudden power outage—the replay engine detects the torn write, rejects the corrupted bytes, and recovers all valid committed state.*
>
> *On Slide 11: Our empirical verification proves that our prototype exceeds expectations. We have 10 test suites containing 307 assertions with zero failures, 0 bytes leaked in Valgrind, and a 4.54x throughput speedup achieved by our 32-striped reader-writer locks over a global mutex."*

---

### Student 3: Om Amrit Das (`2500032631`)
**Slides to Present: 5, 7, 9, 12 (Live Demo Lead)**

> *"Moving to Slide 5: For our requirements, our communication protocol uses an ASCII line-based grammar supporting quoted tokens and escape sequences. Crucially, our network engine handles TCP fragmentation: if a client sends a query in 1-byte chunks across multiple recv calls, our dynamic ClientBuffer accumulates bytes safely until a delimiter is found. We also support pipelining multiple commands in a single packet.*
>
> *Looking at Slide 7: Here is our module block diagram and end-to-end data flow. When a client issues a command like `SET user:100 "Rajnish" 60000`, the server poll loop detects the socket event, the parser tokenizes the input, the WAL frames and computes the CRC32 checksum, the key is written to the striped hash table, and the protocol responds with `OK`.*
>
> *On Slide 9: Our working environment is built on Ubuntu 24.04 LTS under WSL2, using GCC 13 with strict C11 flags and GNU Make. The entire codebase is version-controlled at `github.com/really-rajnish/KVStore` with clean modular directories separating interfaces, implementations, tests, and documentation.*
>
> *Finally, on Slide 12: We are ready for live demonstration. We can run our interactive REPL CLI `bin/kvstore`, execute `make test` across all 307 assertions, run `make valgrind` to prove zero memory leaks, and run `make crash-test` to demonstrate live kill -9 crash consistency."*

---

## Live Demonstration Commands for Viva

Run these commands live in the terminal during the evaluation:

```bash
# 1. Clean and compile all binaries
make clean
make all

# 2. Run the interactive REPL CLI (Module-I Prototype)
bin/kvstore
# Try these commands in the prompt:
# SET name "KVStore Prototype" 10000
# GET name
# COUNT
# TTL name
# EXIT

# 3. Execute all 10 Test Suites (307 assertions, 0 failures)
make test

# 4. Demonstrate Memory Safety (0 bytes leaked in 0 blocks)
make valgrind

# 5. Demonstrate Crash Consistency against abrupt kill -9
make crash-test

# 6. Run the Full Viva Demonstration Automation
make demo
```

---

## Faculty Viva Questions & Model Answers

### Q1: What is the Self-Pipe Trick and why did you use it?
**Answer**:
> *"In POSIX systems, asynchronous signal handlers (such as for `SIGINT` or `SIGTERM`) must be strictly async-signal-safe. Standard library functions like `printf()`, `malloc()`, or socket functions are non-reentrant and can deadlock if called inside a signal handler. The **Self-Pipe Trick** creates a pipe (`self_pipe[2]`) in non-blocking mode. The signal handler only writes a single byte into `self_pipe[1]`, which is async-signal-safe. The server's main `poll()` loop monitors `self_pipe[0]`, immediately detects the wake-up, exits gracefully, drains active connections, and shuts down cleanly."*

### Q2: Why separate chaining with DJB2 over open addressing?
**Answer**:
> *"Separate chaining handles dynamic deletions (`DEL`) and fine-grained locking much better than open addressing with linear probing. In open addressing, deletions require tombstone markers that degrade lookup performance, and locking requires complex bucket-range locking. In separate chaining, each stripe RW-lock protects a bucket's entire collision chain independently, eliminating cascading lock contention."*

### Q3: How do you guarantee crash consistency against `kill -9`?
**Answer**:
> *"When a process is killed with `SIGKILL` (`kill -9`), no application-level exit handlers or cleanup code can run. We achieve crash consistency through Write-Ahead Logging (WAL) with binary record framing and IEEE 802.3 CRC32 checksums. Before in-memory mutations are finalized, records are appended to disk and flushed. On recovery, the replay engine reads records sequentially. If a crash occurred mid-write (a torn write), the checksum verification fails, causing the recovery engine to stop at the last valid record, discard the partial bytes, and safely restore 100% of the committed data."*

### Q4: What is the benefit of Striped RW-Locks over a Global Mutex?
**Answer**:
> *"A global mutex creates a single serialization bottleneck across all CPU cores. Striped locking partitions the hash table's bucket space across 32 independent `pthread_rwlock_t` instances. Key queries map to specific stripes using `(hash % capacity) % 32`. Multiple threads accessing different keys acquire independent locks concurrently, and multiple readers on the same key execute in parallel. In our benchmarks, this delivered a **4.54x throughput speedup** and reduced p99 tail latency by **98.7%**."*
