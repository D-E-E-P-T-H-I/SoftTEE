CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -I. -I../include
AR ?= ar

all: libpex.a

libpex.a: pex.o
	$(AR) rcs $@ $^

pex.o: pex.c pex.h ../include/pex_uapi.h
	$(CC) $(CFLAGS) -c -o $@ pex.c

clean:
	rm -f *.o *.a