CC       := gcc
CFLAGS   := -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -O2 -g -Iinclude -pthread
LDFLAGS  := -pthread

SRC_DIR  := src
BIN_DIR  := bin
TEST_DIR := tests

CORE_SRCS := $(SRC_DIR)/hash_table.c \
             $(SRC_DIR)/wal.c        \
             $(SRC_DIR)/ttl.c        \
             $(SRC_DIR)/compaction.c \
             $(SRC_DIR)/threadpool.c \
             $(SRC_DIR)/kvstore.c   \
             $(SRC_DIR)/parser.c    \
             $(SRC_DIR)/dispatcher.c

REPL_BIN   := $(BIN_DIR)/kvstore
SERVER_BIN := $(BIN_DIR)/kvstore_server
CLIENT_BIN := $(BIN_DIR)/kvstore_client
BENCH_BIN  := $(BIN_DIR)/kvstore_bench

TEST_HT   := $(BIN_DIR)/test_hash_table
TEST_KV   := $(BIN_DIR)/test_kvstore
TEST_PA   := $(BIN_DIR)/test_parser
TEST_PR   := $(BIN_DIR)/test_protocol
TEST_SC   := $(BIN_DIR)/test_server_client
TEST_SG   := $(BIN_DIR)/test_signals
TEST_WL   := $(BIN_DIR)/test_wal
TEST_CP   := $(BIN_DIR)/test_compaction
TEST_CC   := $(BIN_DIR)/test_concurrency
TEST_TTL  := $(BIN_DIR)/test_ttl

TESTS := $(TEST_HT) $(TEST_KV) $(TEST_PA) $(TEST_PR) $(TEST_SC) $(TEST_SG) $(TEST_WL) $(TEST_CP) $(TEST_CC) $(TEST_TTL)
BINS  := $(REPL_BIN) $(SERVER_BIN) $(CLIENT_BIN) $(BENCH_BIN)

.PHONY: all test valgrind tsan crash-test bench demo clean run

all: $(BINS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR) data

$(REPL_BIN): $(CORE_SRCS) $(SRC_DIR)/main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(SERVER_BIN): $(CORE_SRCS) $(SRC_DIR)/protocol.c $(SRC_DIR)/server.c $(SRC_DIR)/server_main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(CLIENT_BIN): $(SRC_DIR)/protocol.c $(SRC_DIR)/client.c $(SRC_DIR)/client_main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BENCH_BIN): $(CORE_SRCS) $(SRC_DIR)/bench_main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_HT): $(SRC_DIR)/hash_table.c $(TEST_DIR)/test_hash_table.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_KV): $(SRC_DIR)/hash_table.c $(SRC_DIR)/wal.c $(SRC_DIR)/ttl.c $(SRC_DIR)/compaction.c $(SRC_DIR)/threadpool.c $(SRC_DIR)/kvstore.c $(TEST_DIR)/test_kvstore.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_PA): $(SRC_DIR)/parser.c $(TEST_DIR)/test_parser.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_PR): $(SRC_DIR)/protocol.c $(TEST_DIR)/test_protocol.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_SC): $(CORE_SRCS) $(SRC_DIR)/protocol.c $(SRC_DIR)/server.c $(SRC_DIR)/client.c $(TEST_DIR)/test_server_client.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_SG): $(CORE_SRCS) $(SRC_DIR)/protocol.c $(SRC_DIR)/server.c $(SRC_DIR)/client.c $(TEST_DIR)/test_signals.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_WL): $(SRC_DIR)/hash_table.c $(SRC_DIR)/wal.c $(SRC_DIR)/ttl.c $(SRC_DIR)/compaction.c $(SRC_DIR)/threadpool.c $(SRC_DIR)/kvstore.c $(TEST_DIR)/test_wal.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_CP): $(SRC_DIR)/hash_table.c $(SRC_DIR)/wal.c $(SRC_DIR)/ttl.c $(SRC_DIR)/compaction.c $(SRC_DIR)/threadpool.c $(SRC_DIR)/kvstore.c $(TEST_DIR)/test_compaction.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_CC): $(SRC_DIR)/hash_table.c $(SRC_DIR)/wal.c $(SRC_DIR)/ttl.c $(SRC_DIR)/compaction.c $(SRC_DIR)/threadpool.c $(SRC_DIR)/kvstore.c $(TEST_DIR)/test_concurrency.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_TTL): $(SRC_DIR)/hash_table.c $(SRC_DIR)/wal.c $(SRC_DIR)/ttl.c $(SRC_DIR)/compaction.c $(SRC_DIR)/threadpool.c $(SRC_DIR)/kvstore.c $(TEST_DIR)/test_ttl.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TESTS)
	@for t in $(TESTS); do echo "== $$t =="; ./$$t || exit 1; done

valgrind: $(TESTS)
	@for t in $(TESTS); do \
		echo "== Valgrind: $$t =="; \
		valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$$t || exit 1; \
	done

tsan: | $(BIN_DIR)
	$(CC) -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -Wextra -fsanitize=thread -g -O1 -Iinclude -pthread \
		$(SRC_DIR)/hash_table.c $(SRC_DIR)/wal.c $(SRC_DIR)/ttl.c $(SRC_DIR)/compaction.c $(SRC_DIR)/threadpool.c $(SRC_DIR)/kvstore.c $(TEST_DIR)/test_concurrency.c \
		-o $(BIN_DIR)/test_concurrency_tsan $(LDFLAGS)
	setarch x86_64 -R ./$(BIN_DIR)/test_concurrency_tsan

crash-test: all
	bash scripts/crash_test.sh

bench: $(BENCH_BIN)
	bash scripts/benchmark.sh

demo: all
	bash scripts/demo.sh

clean:
	rm -rf $(BIN_DIR)/*
