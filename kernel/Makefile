obj-m += pex.o
pex-objs := pex_main.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	@if [ ! -d "$(KDIR)" ]; then \
		echo "ERROR: Kernel build directory not found: $(KDIR)"; \
		echo "Install matching kernel headers or set KDIR explicitly."; \
		echo "Example: make KDIR=/path/to/linux-build -C kernel"; \
		exit 2; \
	fi
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	@if [ -d "$(KDIR)" ]; then \
		$(MAKE) -C $(KDIR) M=$(PWD) clean; \
	else \
		echo "Skipping kernel clean (missing KDIR: $(KDIR))"; \
	fi