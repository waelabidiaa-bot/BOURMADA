CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11
CPPFLAGS := -MMD -MP

SRC_DIR := src
OBJ_DIR := dir
BIN_DIR := bin

TARGET := $(BIN_DIR)/bourmada

SRCS := $(shell find $(SRC_DIR) -name "*.c")
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all benchmark clean

all: $(TARGET)

benchmark: $(TARGET)
	@python3 tests/benchmark.py

$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "Linking executable: $@"
	@$(CC) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	@echo "Cleaning up build artifacts..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

-include $(DEPS)

