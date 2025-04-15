##
# tb_garbage
#
# @file
# @version 0.1

# Root Makefile
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O0 -g -Isrc
CXXFLAGS = $(CFLAGS) -std=c++11
LDFLAGS = -lpthread

# Targets
TARGET = bin/test_gc
SRC_DIR = src
BIN_DIR = bin

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SOURCES))

# Ensure bin directory exists
$(shell mkdir -p $(BIN_DIR))

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
