CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Ilibpex/include
LDFLAGS ?=

.PHONY: all kernel lib examples tests clean load unload run-demo run-tests run-showcase run-e2e

all: lib examples tests

lib:
	$(MAKE) -C libpex

kernel:
	$(MAKE) -C kernel

examples: lib
	$(CC) $(CFLAGS) -o examples/protected_workload examples/protected_workload.c libpex/libpex.a $(LDFLAGS)
	$(CC) $(CFLAGS) -pthread -o examples/showcase_blocking examples/showcase_blocking.c libpex/libpex.a $(LDFLAGS)

tests: lib
	$(CC) $(CFLAGS) -pthread -o tests/test_multithread_violation tests/test_multithread_violation.c libpex/libpex.a $(LDFLAGS)
	$(CC) $(CFLAGS) -o tests/benchmark_entry_exit tests/benchmark_entry_exit.c libpex/libpex.a $(LDFLAGS)

load: kernel
	sudo insmod kernel/pex.ko

unload:
	sudo rmmod pex

run-demo: examples
	./examples/protected_workload

run-showcase: examples
	./examples/showcase_blocking

run-e2e: examples tests kernel
	sudo ./scripts/run_all.sh

run-tests: tests
	./tests/test_multithread_violation
	./tests/benchmark_entry_exit

clean:
	$(MAKE) -C libpex clean
	$(MAKE) -C kernel clean
	rm -f examples/protected_workload tests/test_multithread_violation
	rm -f examples/showcase_blocking
	rm -f tests/benchmark_entry_exit
