CC = gcc
CFLAGS = -g -fprofile-arcs -ftest-coverage

BUILD_DIR = build

SRC_DIR = src
UNIT_TEST_DIR = test/unit
BUILD_UNIT_DIR = build/test/unit

E2E_TEST_DIR = test/e2e
BUILD_E2E_DIR = build/test/e2e
E2E_SCRIPT = test/e2e/test.sh

SRC_FILES = $(SRC_DIR)/*.c
UNIT_TEST_FILES = $(UNIT_TEST_DIR)/*.c
E2E_TEST_FILES = $(E2E_TEST_DIR)/*.c

TEST_PROGRAM = tests_program
ANALYZE_PROGRAM = analyze_program

all: regression

build_test:
	$(CC) $(CFLAGS) -o $(BUILD_UNIT_DIR)/$(TEST_PROGRAM) $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	$(CC) $(CFLAGS) -o $(BUILD_E2E_DIR)/test1 $(E2E_TEST_DIR)/normal_scenario.c $(SRC_FILES) -lz -lcunit
	$(CC) $(CFLAGS) -o $(BUILD_E2E_DIR)/test2 $(E2E_TEST_DIR)/seq_fault_test.c $(SRC_FILES) -lz -lcunit

test: build_test
	$(BUILD_UNIT_DIR)/$(TEST_PROGRAM)
	$(E2E_SCRIPT)
	
regression: test analyze

analyze: build_test
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --error-exitcode=1 $(BUILD_UNIT_DIR)/$(TEST_PROGRAM)
	clang-tidy --quiet -checks=bugprone-*,-bugprone-easily-swappable-parameters,clang-analyzer-*,cert-*,concurrency-*,misc-*,modernize-*,performance-*,readability-* --warnings-as-errors=* ./src/allocator.c
	scan-build --status-bugs --keep-cc --show-description $(BUILD_UNIT_DIR)/$(TEST_PROGRAM)
	clang --analyze -Xanalyzer -analyzer-output=text ./$(BUILD_UNIT_DIR)/$(TEST_PROGRAM)
	clang -fsanitize=address -o program $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	./program
	rm -f ./program

	clang -fsanitize=thread -o program $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	./program
	rm -f ./program

	clang -fsanitize=memory -o program $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	./program
	rm -f ./program

	clang -fsanitize=undefined -o program $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	./program
	rm -f ./program

	clang -flto -fsanitize=cfi -fvisibility=default -o program $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	./program
	rm -f ./program

	clang -flto -fsanitize=safe-stack -o program $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	./program
	rm -f ./program

	# clang -flto -fsanitize=dataflow -o program $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	# ./program
	# rm -f ./program

clean:
	rm -f $(BUILD_DIR)/$(TEST_PROGRAM) $(BUILD_DIR)/$(ANALYZE_PROGRAM) $(BUILD_DIR)/*.gc* $(BUILD_DIR)/build/*.o
	rm -f $(BUILD_E2E_DIR)/* $(BUILD_UNIT_DIR)/*

