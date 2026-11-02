
#/**
#* @file    Makefile
#* @author  Jakub Poláček (xpolacj00)
#* @brief   Makefile
#* @date    2026-09-26
#*
#* @details run clean build check build-check test run_valgrind cppcheck tidy tidy-fix format format-fix editorconfig
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

check: tidy format editorconfig

tidy: $(SRC)
	@if [ -z "$(SRC)" ]; then \
		echo "No C sources found, skipping clang-tidy"; \
	elif command -v $(TIDY) >/dev/null 2>&1; then \
		echo "Starting clang-tidy..."; \
		$(TIDY) $(SRC) -- $(CFLAGS); \
	else \
		echo "Warning: clang-tidy is not installed"; \
	fi

tidy-fix: $(SRC)
	@if [ -z "$(SRC)" ]; then \
		echo "No C sources found, skipping clang-tidy"; \
	elif command -v $(TIDY) >/dev/null 2>&1; then \
		echo "Starting clang-tidy..."; \
		$(TIDY) $(SRC) --fix --fix-errors -- $(CFLAGS); \
	else \
		echo "Warning: clang-tidy is not installed"; \
	fi

format: $(SRC) $(HEADERS)
	@if [ -z "$(SRC)$(HEADERS)" ]; then \
		echo "No C sources or headers found, skipping clang-format"; \
	elif command -v $(FORMAT) >/dev/null 2>&1; then \
		echo "Starting clang-format..."; \
		$(FORMAT) --dry-run --Werror $(SRC) $(HEADERS); \
	else \
		echo "Warning: clang-format is not installed"; \
	fi

format-fix: $(SRC) $(HEADERS)
	@if [ -z "$(SRC)$(HEADERS)" ]; then \
		echo "No C sources or headers found, skipping clang-format"; \
	elif command -v $(FORMAT) >/dev/null 2>&1; then \
		echo "Formatting files..."; \
		$(FORMAT) -i --verbose $(SRC) $(HEADERS); \
	else \
		echo "Warning: clang-format is not installed"; \
	fi

editorconfig:
	@if command -v editorconfig-checker >/dev/null 2>&1; then \
		echo "Starting editorconfig-checker..."; \
		editorconfig-checker -format gcc; \
	else \
		echo "Error: editorconfig-checker is not installed"; \
		exit 1; \
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

.PHONY: run clean build check build-check cppcheck tidy tidy-fix format format-fix test run_valgrind editorconfig
