CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Iinclude
AR ?= ar

LIB = libpex.a
OBJ = src/pex.o

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

src/pex.o: src/pex.c include/pex.h include/pex_uapi.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(LIB)
