CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -I./include -I./libpex -pthread

all: kernel lib examples tests
user: lib examples tests

kernel:
	$(MAKE) -C kernel

lib:
	$(MAKE) -C libpex

examples: lib
	$(CC) $(CFLAGS) -o examples/demo examples/demo.c libpex/libpex.a

tests: lib
	$(CC) $(CFLAGS) -o tests/test_faults tests/test_faults.c libpex/libpex.a

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C libpex clean
	rm -f examples/demo tests/test_faults

.PHONY: all user kernel lib examples tests clean