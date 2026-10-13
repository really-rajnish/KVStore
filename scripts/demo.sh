#!/usr/bin/env bash
set -e

PORT=18291
WAL_FILE="data/demo.wal"
rm -f "$WAL_FILE" "$WAL_FILE.compact.tmp"

echo "===================================================================="
echo "    KVSTORE OS CAPSTONE VIVA DEMONSTRATION (Weeks 1 - 12)          "
echo "    KL University - Course 25CS2104E - Distinction Bar Evaluation   "
echo "===================================================================="
sleep 1

echo ""
echo "[Step 1] Building All Deliverables Cleanly..."
make clean > /dev/null
make all
echo "Build succeeded: bin/kvstore, bin/kvstore_server, bin/kvstore_client, bin/kvstore_bench."

echo ""
echo "[Step 2] Launching Multi-Threaded TCP Server on port $PORT with WAL $WAL_FILE..."
./bin/kvstore_server -p $PORT -d "$WAL_FILE" &
SERVER_PID=$!
sleep 1
echo "Server running with PID $SERVER_PID."

echo ""
echo "[Step 3] Executing Remote TCP Client Commands..."
echo "-> SET user:101 'Alice Smith'"
./bin/kvstore_client -p $PORT 'SET user:101 "Alice Smith"'
echo "-> GET user:101"
./bin/kvstore_client -p $PORT "GET user:101"
echo "-> COUNT"
./bin/kvstore_client -p $PORT "COUNT"

echo ""
echo "[Step 4] Demonstrating Key TTL & Expiration Engine..."
echo "-> SET session:temp 'ActiveSessionToken' 300 (TTL 300ms)"
./bin/kvstore_client -p $PORT "SET session:temp 'ActiveSessionToken' 300"
echo "-> Query TTL immediately:"
./bin/kvstore_client -p $PORT "TTL session:temp"
echo "Sleeping 400ms for expiration..."
sleep 0.4
echo "-> Query TTL after 400ms:"
./bin/kvstore_client -p $PORT "TTL session:temp"
echo "-> GET session:temp (Lazy eviction verification):"
./bin/kvstore_client -p $PORT "GET session:temp"

echo ""
echo "[Step 5] Demonstrating Online Atomic Log Compaction..."
echo "Appending 50 mutations to grow the WAL..."
for i in $(seq 1 50); do
    ./bin/kvstore_client -p $PORT "SET churn_key 'value_$i'" > /dev/null
done
PRE_SIZE=$(stat -c%s "$WAL_FILE")
echo "WAL size before compaction: $PRE_SIZE bytes."
echo "-> Issuing COMPACT command..."
./bin/kvstore_client -p $PORT "COMPACT"
POST_SIZE=$(stat -c%s "$WAL_FILE")
echo "WAL size after compaction:  $POST_SIZE bytes."

echo ""
echo "[Step 6] Demonstrating Durability & Crash Consistency (kill -9)..."
./bin/kvstore_client -p $PORT "SET critical_data 'DurablePayloadSurvivesCrash'"
echo "Delivering SIGKILL (kill -9) to server PID $SERVER_PID..."
kill -9 $SERVER_PID 2>/dev/null || true
sleep 1
echo "Server abruptly terminated."

echo "Relaunching server from WAL $WAL_FILE..."
./bin/kvstore_server -p $PORT -d "$WAL_FILE" &
RECOVERED_PID=$!
sleep 1
echo "Server recovered with PID $RECOVERED_PID."
echo "-> GET critical_data from recovered store:"
./bin/kvstore_client -p $PORT "GET critical_data"

echo ""
echo "[Step 7] Demonstrating Signal-Safe Graceful Shutdown..."
echo "Sending SIGTERM to recovered server PID $RECOVERED_PID..."
kill -15 $RECOVERED_PID
wait $RECOVERED_PID 2>/dev/null || true
echo "Server drained in-flight requests and cleanly shut down."

echo ""
echo "[Step 8] Demonstrating Concurrency Speedup (Striped RW-Locks vs Global Mutex)..."
./bin/kvstore_bench -t 8 -n 15000 -k 1000 -r 85

echo ""
echo "===================================================================="
echo "    ALL CAPSTONE DELIVERABLES & DISTINCTION BARS DEMONSTRATED!      "
echo "===================================================================="
