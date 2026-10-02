CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -O2 -g -Iinclude
LDFLAGS  :=

SRC_DIR  := src
BIN_DIR  := bin
TEST_DIR := tests

CORE_SRCS := $(SRC_DIR)/hash_table.c $(SRC_DIR)/kvstore.c
APP_SRCS  := $(CORE_SRCS) $(SRC_DIR)/main.c
APP_BIN   := $(BIN_DIR)/kvstore
TEST_HT   := $(BIN_DIR)/test_hash_table

.PHONY: all test clean run

all: $(APP_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR) data

$(APP_BIN): $(APP_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(APP_SRCS) -o $@ $(LDFLAGS)

$(TEST_HT): $(CORE_SRCS) $(TEST_DIR)/test_hash_table.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CORE_SRCS) $(TEST_DIR)/test_hash_table.c -o $@ $(LDFLAGS)

test: $(TEST_HT)
	./$(TEST_HT)

run: $(APP_BIN)
	./$(APP_BIN)

clean:
	rm -f $(BIN_DIR)/*
