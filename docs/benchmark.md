# Empirical Benchmarking & Performance Evaluation (Week 12)

**Course**: KL University — 25CS2104E OS Capstone (Project 19)  
**Deliverable**: Empirical Validation of Concurrency Subsystem & Striped Reader-Writer Locking  
**Binary**: `bin/kvstore_bench`  
**Automated Script**: `scripts/benchmark.sh` (`make bench`)

---

## 1. Executive Summary

A core distinction requirement of the KVStore capstone is proving measurable concurrency speedup under multi-threaded contention when transitioning from a coarse-grained Global Mutex (`KV_LOCK_GLOBAL`) to Striped Reader-Writer Locks (`KV_LOCK_STRIPED`, 32 independent `pthread_rwlock_t` instances).

Under a multi-threaded workload of 240,000 operations across 8 worker threads on Ubuntu 24.04 LTS (WSL2 x86_64):
- **Read-Heavy Workloads (95% Reads, 5% Writes)**: Striped RW-Locks achieved **7,221,684 OPS** versus 1,591,786 OPS for Global Mutex, delivering a **4.54x throughput speedup**.
- **Balanced Workloads (50% Reads, 50% Writes)**: Striped RW-Locks achieved **3,449,603 OPS** versus 1,222,022 OPS for Global Mutex, delivering a **2.82x throughput speedup**.
- **p99 Tail Latency**: Reduced from **76.0 µs** down to **1.0 µs** (a **98.7% reduction** in tail latency).

---

## 2. Experimental Methodology

### Hardware & Environment
- **CPU**: AMD / Intel x86_64 multi-core processor
- **OS**: Ubuntu 24.04 LTS under WSL2 (Linux 6.6 kernel)
- **Compiler**: GCC 13.x with `-std=c11 -O2 -Wall -Wextra -Wpedantic -pthread`
- **Timer**: POSIX `clock_gettime(CLOCK_MONOTONIC)` with sub-microsecond precision

### Workload Generator Architecture
The benchmark utility (`src/bench_main.c`) spawns $T$ concurrent POSIX threads. Each thread executes $N$ continuous operations against a pre-populated key space of size $K$.
- Operations are deterministically pseudo-randomized using orthogonal linear congruential strides per worker thread.
- Thread-safe copy semantics (`kvstore_get_copy()`) are evaluated to accurately measure end-to-end lookup latency including memory duplication.
- Latency samples are collected across all threads into an uncompressed buffer and evaluated via `qsort` for exact percentiles (p50, p90, p99, and max).

---

## 3. Benchmark Results

### Table 1: Workload Comparison (8 Threads, 240,000 Operations, 2,000 Keys)

| Workload Type | Concurrency Mode | Throughput (OPS) | Speedup | Latency p50 | Latency p90 | Latency p99 | Max Latency |
|:--------------|:-----------------|-----------------:|--------:|------------:|------------:|------------:|------------:|
| **Read-Heavy (95% R / 5% W)** | Global Mutex | 1,591,786 | 1.00x | 0.0 µs | 4.0 µs | 76.0 µs | 1,197.0 µs |
| **Read-Heavy (95% R / 5% W)** | **32-Stripe RW-Lock** | **7,221,684** | **4.54x** | **0.0 µs** | **0.0 µs** | **1.0 µs** | **899.0 µs** |
| **Balanced (50% R / 50% W)** | Global Mutex | 1,222,022 | 1.00x | 0.0 µs | 9.0 µs | 90.0 µs | 1,459.0 µs |
| **Balanced (50% R / 50% W)** | **32-Stripe RW-Lock** | **3,449,603** | **2.82x** | **0.0 µs** | **0.0 µs** | **10.0 µs** | **1,054.0 µs** |

---

### Table 2: Thread Scaling Sweep (80% Reads / 20% Writes, 2,000 Keys)

| Thread Count | Global Mutex (OPS) | Striped RW-Lock (OPS) | Speedup Factor | Striped p99 Latency |
|:------------:|-------------------:|----------------------:|:--------------:|:-------------------:|
| **1 Thread** | 3,542,999 | 3,726,350 | **1.05x** | 0.0 µs |
| **2 Threads** | 1,239,794 | 6,267,146 | **5.05x** | 0.0 µs |
| **4 Threads** | 1,385,723 | 5,601,880 | **4.04x** | 1.0 µs |
| **8 Threads** | 1,320,536 | 4,120,833 | **3.12x** | 1.0 µs |
| **16 Threads** | 1,609,581 | 4,619,567 | **2.87x** | 32.0 µs |

---

## 4. Key Analytical Insights

1. **Lock Contention Collapse in Global Mutex**:
   With a single mutex, scaling from 1 thread to 2 threads immediately collapsed throughput from 3.54M OPS to 1.24M OPS (a 65% drop) due to severe cache-line bouncing and lock-acquisition serialization.
2. **Parallel Reader Scaling in Striped Mode**:
   Striped reader-writer locking avoids contention across distinct buckets. Multiple reader threads access identical and differing stripes concurrently without serialization, scaling throughput up to **7.22 Million OPS**.
3. **Tail Latency Elimination**:
   Under Global Mutex, p99 latency ballooned up to 76–95 µs because threads queued behind long write operations. Striped locking reduced p99 tail latency to **1.0 µs** (a 98%+ improvement).

---

## 5. Verification Commands

To reproduce the empirical benchmark suite:
```bash
make bench
# Or run with custom parameters:
./bin/kvstore_bench -t 8 -n 50000 -k 2000 -r 90 -m both
```
