cat > README.md << 'EOF'
# KVStore — A Persistent Key-Value Store

An in-memory key-value store that survives restarts: a hash index, an
append-only log, compaction, and a wire protocol.

**Stack:** Linux (WSL2) · C11 · TCP · pthreads

## Team

| Name | Roll No. | Contribution (Week 1) |
|------|----------|-----------------------|
| <student 1> | <roll> | Hash table + KVStore API |
| <student 2> | <roll> | REPL driver + Makefile |
| <student 3> | <roll> | Docs + test suite |

## Problem Statement

Applications need fast, durable key-value storage without the weight of a
full database. This project builds one from scratch on Linux, applying core
OS concepts — processes, file descriptors, memory management, threads, and
synchronisation — at each stage of the 12-week syllabus.

## Objectives

1. Implement an O(1) average-case hash index with GET / SET / DEL.
2. Add durability through an append-only log and startup replay.
3. Serve multiple concurrent clients over TCP with correct locking.
4. Support log compaction, snapshots, and TTL expiry.
5. Benchmark global vs. striped locking under load.

## Scope

- **In scope:** single-node server, in-memory index, append-only persistence,
  concurrent TCP clients, compaction, TTL, benchmarking.
- **Out of scope:** replication, clustering, distribution, authentication.

## Planned Features

- [x] In-memory hash table (Week 1)
- [x] GET / SET / DEL via REPL (Week 1)
- [ ] Command parser (Week 3)
- [ ] TCP server and client (Week 4)
- [ ] Command dispatcher (Week 5)
- [ ] Signal-safe shutdown with log flush (Week 6)
- [ ] Append-only log + startup replay (Week 7)
- [ ] Valgrind-clean under churn (Week 8)
- [ ] Log compaction + snapshots (Week 9)
- [ ] Concurrent clients with striped locking (Week 10)
- [ ] TTL expiry under concurrency (Week 11)
- [ ] Packaging, benchmark, demo (Week 12)

## Build and Run

    make            # builds bin/kvstore
    ./bin/kvstore   # starts the REPL
    make test       # runs the unit tests

### REPL commands

    SET key value
    GET key
    DEL key
    COUNT
    EXIT

## Project Structure

    KVStore/
    ├── include/    public headers
    ├── src/        implementation
    ├── tests/      unit tests
    ├── docs/       requirements, architecture, design, testing
    ├── bin/        build output
    └── data/       append-only log (later)

## Environment

- Ubuntu 24.04 on WSL2
- gcc (C11), GNU Make, Git
- Valgrind, ThreadSanitizer

## Current Stage

Core Module I — in-memory hash table with GET/SET/DEL via REPL.
EOF
cat README.md



## Current Stage

Core Module I — hash table + KVStore API + parser + dispatcher + REPL.

- [x] In-memory hash table (Week 1)
- [x] KVStore API (Week 1)
- [x] REPL (Week 1)
- [x] Protocol parser (Week 3)
- [x] Command dispatcher (Week 3)
- [ ] TCP server and client (Week 4)
- [ ] Signal-safe shutdown (Week 6)
- [ ] Append-only log + startup replay (Week 7)
- [ ] Log compaction + snapshots (Week 9)
- [ ] Concurrent clients with striped locking (Week 10)
- [ ] TTL expiry under concurrency (Week 11)
- [ ] Packaging + benchmark + demo (Week 12)
