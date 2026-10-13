#!/usr/bin/env bash
set -e

mkdir -p bin data docs

echo "=========================================================="
echo " Running Empirical Benchmark Matrix (Striped vs Global)  "
echo "=========================================================="

echo ""
echo "--- Test 1: Read-Heavy Workload (95% Reads, 5% Writes) ---"
./bin/kvstore_bench -t 8 -n 30000 -k 2000 -r 95

echo ""
echo "--- Test 2: Balanced Workload (50% Reads, 50% Writes) ---"
./bin/kvstore_bench -t 8 -n 30000 -k 2000 -r 50

echo ""
echo "--- Test 3: Thread Scaling Sweep (1, 2, 4, 8, 16 threads, 80% Reads) ---"
for th in 1 2 4 8 16; do
    echo ">> Threads: $th"
    ./bin/kvstore_bench -t $th -n 20000 -k 2000 -r 80
done

echo ""
echo "Benchmark suite completed successfully!"
