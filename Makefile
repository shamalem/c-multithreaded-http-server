CC      := gcc
CSTD    := -std=c17
WARN    := -Wall -Wextra -Wpedantic -Werror
CFLAGS  := $(CSTD) $(WARN) -g -Iinclude
LDFLAGS := -pthread

SRC_DIR   := src
BUILD_DIR := build
BIN       := server

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run asan

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

clean:
	rm -rf $(BUILD_DIR) $(BIN)
