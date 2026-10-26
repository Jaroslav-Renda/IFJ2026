
#/**
#* @file    Makefile
#* @author  Jakub Poláček (xpolacj00)
#* @brief   Makefile
#* @date    2026-09-26
#* 
#* @details run clean build check build-check test run_valgrind cppcheck tidy format
#*                 
#*/

CC=gcc
CFLAGS= -pedantic -Wall -Wextra -g -O0 -std=c23
# CI passes FORMAT=clang-format-23 and TIDY=clang-tidy-23; fall back to
# unpinned binaries locally.
FORMAT?= clang-format
TIDY?= clang-tidy
HEADERS = $(wildcard *.h)
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

build: $(OBJ)
	$(CC) -o compiler $^

build-check: check build

check: cppcheck tidy

cppcheck: $(SRC)
	@if [ -z "$(SRC)" ]; then \
		echo "No C sources found, skipping cppcheck"; \
	elif command -v cppcheck >/dev/null 2>&1; then \
		echo "Starting cppcheck"; \
		cppcheck --enable=all --error-exitcode=1 $(SRC); \
	else \
		echo "Warning: cppcheck is not installed"; \
	fi

tidy: $(SRC)
	@if [ -z "$(SRC)" ]; then \
		echo "No C sources found, skipping clang-tidy"; \
	elif command -v $(TIDY) >/dev/null 2>&1; then \
		echo "Starting clang-tidy..."; \
		$(TIDY) $(SRC) -- $(CFLAGS); \
	else \
		echo "Warning: clang-tidy is not installed"; \
	fi

format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Formatting files..."; \
		clang-format -i --verbose $(SRC) $(HEADERS); \
	else \
		echo "Warning: clang-format is not installed"; \
	fi

test:
	mkdir ./test_f

run: build
	./compiler

run_valgrind: build
	valgrind --leak-check=full --track-origins=yes ./compiler

clean:
	rm -f *.o compiler

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: run clean build check build-check cppcheck tidy test run_valgrind
