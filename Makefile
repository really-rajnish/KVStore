CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -O2 -g -Iinclude
LDFLAGS  :=

SRC_DIR  := src
BIN_DIR  := bin
TEST_DIR := tests

CORE_SRCS := $(SRC_DIR)/hash_table.c \
             $(SRC_DIR)/kvstore.c   \
             $(SRC_DIR)/parser.c    \
             $(SRC_DIR)/dispatcher.c

APP_SRCS  := $(CORE_SRCS) $(SRC_DIR)/main.c
APP_BIN   := $(BIN_DIR)/kvstore

TEST_HT   := $(BIN_DIR)/test_hash_table
TEST_KV   := $(BIN_DIR)/test_kvstore
TEST_PA   := $(BIN_DIR)/test_parser

TESTS := $(TEST_HT) $(TEST_KV) $(TEST_PA)

.PHONY: all test clean run

all: $(APP_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR) data

$(APP_BIN): $(APP_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(APP_SRCS) -o $@ $(LDFLAGS)

$(TEST_HT): $(SRC_DIR)/hash_table.c $(TEST_DIR)/test_hash_table.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_KV): $(SRC_DIR)/hash_table.c $(SRC_DIR)/kvstore.c $(TEST_DIR)/test_kvstore.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_PA): $(SRC_DIR)/parser.c $(TEST_DIR)/test_parser.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TESTS)
	@for t in $(TESTS); do echo "== $$t =="; ./$$t || exit 1; done

run: $(APP_BIN)
	./$(APP_BIN)

clean:
	rm -f $(BIN_DIR)/*
