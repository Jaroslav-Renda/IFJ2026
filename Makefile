
#/**
#* @file    Makefile
#* @author  Jakub Poláček (xpolacj00)
#* @brief   Makefile
#* @date    2026-09-26
#* 
#* @details run clean build check build-check test run_valgrind
#*                 
#*/

CC=gcc
CFLAGS= -pedantic -Wall -Wextra -g -O0 -std=c23
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

build: $(OBJ)
	$(CC) -o compiler $^

check: *.c
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "Starting the check"; \
		cppcheck --enable=all $(SRC); \
	else \
		echo "Warning: cppcheck is not installed";\
	fi
build-check: check build

test: 
	mkdir ./test_f
	
run: build
	./compiler

run_valgrind: build
	valgrind --leak-check=full --track-origins=yes ./compiler

clean:
	rm *.o compiler

%.o: %c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: run clean build check build-check test run_valgrind