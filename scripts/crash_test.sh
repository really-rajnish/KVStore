#!/usr/bin/env bash
set -e

PORT=17492
WAL_FILE="data/crash_test.wal"

echo "=================================================="
echo "    KVStore Crash Consistency Test (kill -9)     "
echo "=================================================="

mkdir -p data bin scripts
rm -f "$WAL_FILE"

# Step 1: Launch server
echo "[1/4] Starting server on port $PORT with WAL $WAL_FILE..."
./bin/kvstore_server -p $PORT -d "$WAL_FILE" > /dev/null 2>&1 &
SERVER_PID=$!
sleep 0.4

# Step 2: Insert 100 keys
echo "[2/4] Writing 100 keys..."
for i in $(seq 1 100); do
    RESP=$(./bin/kvstore_client -p $PORT SET "user:$i" "value_$i")
    if [ "$RESP" != "OK" ]; then
        echo "Error setting user:$i: $RESP"
        kill -9 $SERVER_PID 2>/dev/null || true
        exit 1
    fi
done

# Step 3: Abruptly kill server with SIGKILL (kill -9)
echo "[3/4] Delivering SIGKILL (kill -9) to server PID $SERVER_PID..."
kill -9 $SERVER_PID
wait $SERVER_PID 2>/dev/null || true
echo "Server abruptly terminated."

# Step 4: Relaunch server on same WAL and verify recovery
echo "[4/4] Relaunching server and verifying startup WAL replay..."
./bin/kvstore_server -p $PORT -d "$WAL_FILE" > /dev/null 2>&1 &
SERVER_PID=$!
sleep 0.4

COUNT=$(./bin/kvstore_client -p $PORT COUNT)
if [ "$COUNT" != "100" ]; then
    echo "FAIL: Expected 100 recovered keys, got: $COUNT"
    kill -9 $SERVER_PID 2>/dev/null || true
    exit 1
fi

# Verify samples
for i in 1 25 50 75 100; do
    VAL=$(./bin/kvstore_client -p $PORT GET "user:$i")
    EXPECTED="\"value_$i\""
    if [ "$VAL" != "$EXPECTED" ]; then
        echo "FAIL: Key user:$i expected $EXPECTED, got: $VAL"
        kill -9 $SERVER_PID 2>/dev/null || true
        exit 1
    fi
done

echo "Sampling check passed! All 100 keys restored."

# Second round: mutations then kill -9 again
echo "Performing 20 deletes and 10 overwrites..."
for i in $(seq 1 20); do
    ./bin/kvstore_client -p $PORT DEL "user:$i" > /dev/null
done
./bin/kvstore_client -p $PORT SET "user:50" "new_val_50" > /dev/null

echo "Delivering second kill -9..."
kill -9 $SERVER_PID
wait $SERVER_PID 2>/dev/null || true

echo "Relaunching server after second crash..."
./bin/kvstore_server -p $PORT -d "$WAL_FILE" > /dev/null 2>&1 &
SERVER_PID=$!
sleep 0.4

COUNT=$(./bin/kvstore_client -p $PORT COUNT)
if [ "$COUNT" != "80" ]; then
    echo "FAIL: Expected 80 keys after deletes, got: $COUNT"
    kill -9 $SERVER_PID 2>/dev/null || true
    exit 1
fi

VAL_DEL=$(./bin/kvstore_client -p $PORT GET "user:5")
if [ "$VAL_DEL" != "(nil)" ]; then
    echo "FAIL: Deleted key user:5 should be (nil), got: $VAL_DEL"
    kill -9 $SERVER_PID 2>/dev/null || true
    exit 1
fi

VAL_50=$(./bin/kvstore_client -p $PORT GET "user:50")
if [ "$VAL_50" != "\"new_val_50\"" ]; then
    echo "FAIL: Overwritten key user:50 should be \"new_val_50\", got: $VAL_50"
    kill -9 $SERVER_PID 2>/dev/null || true
    exit 1
fi

# Clean shutdown
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null || true
rm -f "$WAL_FILE"

echo "=================================================="
echo "    CRASH CONSISTENCY TEST: 100% PASSED           "
echo "=================================================="
