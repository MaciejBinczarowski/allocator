CC = gcc
CFLAGS = -g -Isrc/include

BUILD_DIR = build

SRC_DIR = src
UNIT_TEST_DIR = test/unit
BUILD_UNIT_DIR = build/test/unit

EXAMPLE_FILES = examples/*.c
BUILD_EXAMPLES_DIR = build/examples

E2E_TEST_DIR = test/e2e
BUILD_E2E_DIR = build/test/e2e
E2E_SCRIPT = test/e2e/test.py

SRC_FILES = $(SRC_DIR)/*.c
UNIT_TEST_FILES = $(UNIT_TEST_DIR)/*.c
E2E_TEST_FILES = $(E2E_TEST_DIR)/*.c

TEST_PROGRAM = tests_program
ANALYZE_PROGRAM = analyze_program

all: build_examples build_test

build_examples:
	mkdir -p $(BUILD_EXAMPLES_DIR)

	for file in $(EXAMPLE_FILES); do \
		# basename --suffix $$file - removes the .c extension and path from the file name \
		$(CC) $(CFLAGS) -o $(BUILD_EXAMPLES_DIR)/$$(basename --suffix=.c $$file).o $$file $(SRC_FILES) -lz -lcunit; \
	done

build_test:
	mkdir -p $(BUILD_UNIT_DIR) $(BUILD_E2E_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_UNIT_DIR)/$(TEST_PROGRAM) $(UNIT_TEST_FILES) $(SRC_FILES) -lz -lcunit
	$(CC) $(CFLAGS) -o $(BUILD_E2E_DIR)/correct_scenario $(E2E_TEST_DIR)/normal_scenario.c $(SRC_FILES) -lz -lcunit
	$(CC) $(CFLAGS) -o $(BUILD_E2E_DIR)/seq_fault_test $(E2E_TEST_DIR)/seq_fault_test.c $(SRC_FILES) -lz -lcunit

test: build_test
	$(BUILD_UNIT_DIR)/$(TEST_PROGRAM)
	python3 $(E2E_SCRIPT)
	
examples: build_examples
	for file in $(BUILD_EXAMPLES_DIR)/*.o; do \
		echo "-----------------------------------"; \
		echo "Running example: $$file"; \
		$$file; \
		echo "Example $$file finished"; \
	done

regression: test analyze clean

analyze: build_test
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --error-exitcode=1 $(BUILD_UNIT_DIR)/$(TEST_PROGRAM)
	clang-tidy --quiet -checks=bugprone-*,-bugprone-easily-swappable-parameters,clang-analyzer-*,cert-*,concurrency-*,misc-*,modernize-*,performance-*,readability-* --warnings-as-errors=* ./src/allocator.c --extra-arg=-I./src/include/
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

install:
	sudo chmod a+x ./scripts/install_lib/install_lib.sh ./scripts/install_env/env-preparation.sh
	./scripts/install_env/env-preparation.sh
	./scripts/install_lib/install_lib.sh $(DESTINATION)

clean:
	rm -fr build

