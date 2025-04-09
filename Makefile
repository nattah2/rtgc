##
# tb_garbage
#
# @file
# @version 0.1

CC = gcc
CFLAGS = -Wall -Wextra -I src  # Add -I src to include headers
LDFLAGS =

# Directories
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

# Source and test files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))
TEST_FILES = $(wildcard $(TEST_DIR)/*.c)

# Output binaries
TARGET = program
TEST_TARGET = test_runner

# Create build directory if not exists
$(shell mkdir -p $(BUILD_DIR))

# Default target
all: $(TARGET)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the final program
$(TARGET): $(OBJ_FILES)
	$(CC) $(LDFLAGS) $^ -o $@

# Compile and run tests
test: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(TEST_FILES) $(OBJ_FILES) -o $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)


# end
