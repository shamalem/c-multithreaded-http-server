CC      := gcc
CSTD    := -std=c17
WARN    := -Wall -Wextra -Wpedantic -Werror
CFLAGS  := $(CSTD) $(WARN) -D_POSIX_C_SOURCE=200809L -g -Iinclude
LDFLAGS := -pthread

SRC_DIR   := src
BUILD_DIR := build
BIN       := server

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TEST_BIN := test_http

.PHONY: all clean run asan test

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build with AddressSanitizer/UBSan for local bug-hunting.
# Don't combine with Valgrind runs (both instrument allocations).
asan: CFLAGS += -fsanitize=address,undefined
asan: LDFLAGS += -fsanitize=address,undefined
asan: clean $(BIN)

run: all
	./$(BIN)

# Unit tests link only the pure logic modules (http.c) plus the test
# file itself -- deliberately not server.c/thread_pool.c, since those
# need a live socket/threads rather than being unit-testable.
test: | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_http.c $(SRC_DIR)/http.c -o $(BUILD_DIR)/$(TEST_BIN)
	./$(BUILD_DIR)/$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN)
