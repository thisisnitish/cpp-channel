# Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

# Binaries
BINARIES = example channel_test select_test

# Source files
example_SRC = $(SRC_DIR)/main.cpp
channel_test_SRC = $(TEST_DIR)/channel_tests.cpp
select_test_SRC = $(TEST_DIR)/select_tests.cpp

# Object files
example_OBJ = $(BUILD_DIR)/main.o
channel_test_OBJ = $(BUILD_DIR)/channel_tests.o
select_test_OBJ = $(BUILD_DIR)/select_tests.o

all: $(BUILD_DIR) $(BINARIES)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Targets
example: $(example_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $(BUILD_DIR)/$@

channel_test: $(channel_test_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $(BUILD_DIR)/$@

select_test: $(select_test_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $(BUILD_DIR)/$@

# Compile rule
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
