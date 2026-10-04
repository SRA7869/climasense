.RECIPEPREFIX = >
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -pthread
SRC      = $(wildcard src/*.cpp)
LIB_SRC  = $(filter-out src/main.cpp,$(SRC))
TEST_SRC = $(wildcard tests/*.cpp)
HEADERS  = $(wildcard include/*.h) $(wildcard tests/*.h)
TARGET   = climasense
TEST_BIN = run_tests

$(TARGET): $(SRC) $(HEADERS)
> $(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
> ./$(TARGET)

$(TEST_BIN): $(LIB_SRC) $(TEST_SRC) $(HEADERS)
> $(CXX) $(CXXFLAGS) $(LIB_SRC) $(TEST_SRC) -o $(TEST_BIN)

test: $(TEST_BIN)
> ./$(TEST_BIN)

asan:
> $(CXX) $(CXXFLAGS) -g -fsanitize=address,undefined $(LIB_SRC) $(TEST_SRC) -o $(TEST_BIN)_asan
> ./$(TEST_BIN)_asan

tsan:
> $(CXX) $(CXXFLAGS) -g -fsanitize=thread $(SRC) -o $(TARGET)_tsan
> ./$(TARGET)_tsan

clean:
> rm -f $(TARGET) $(TEST_BIN) $(TEST_BIN)_asan $(TARGET)_tsan test_tmp.conf

.PHONY: run test asan tsan clean
