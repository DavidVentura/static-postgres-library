# Makefile for building PostgreSQL embedded examples

# Configuration
CC = musl-gcc
CFLAGS = -static -I"../src" -I"../vendor/pg18/src/include" -O2 -g
LDFLAGS = -Wl,--gc-sections
LIBPOSTGRES = ../src/libpostgres.a

# Targets
EXAMPLES = example initdb

.PHONY: all clean

all: $(EXAMPLES)

# Build example binary
example: example.c $(LIBPOSTGRES)
	$(CC) $(CFLAGS) $< $(LIBPOSTGRES) $(LDFLAGS) -o $@

# Build initdb binary
initdb: initdb.c $(LIBPOSTGRES)
	$(CC) $(CFLAGS) $< $(LIBPOSTGRES) $(LDFLAGS) -o $@

clean:
	rm -f $(EXAMPLES)
