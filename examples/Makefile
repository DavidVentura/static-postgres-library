CC = musl-gcc
CFLAGS = -static -I"../src" -I"../vendor/pg18/src/include" -O0 -ggdb
LDFLAGS = -Wl,--gc-sections
LIBPOSTGRES = ../src/libpostgres.a

EXAMPLES = example initdb reopen

.PHONY: all clean

all: $(EXAMPLES)

reopen: reopen.c $(LIBPOSTGRES)
	$(CC) $(CFLAGS) $< $(LIBPOSTGRES) $(LDFLAGS) -o $@

example: example.c $(LIBPOSTGRES)
	$(CC) $(CFLAGS) $< $(LIBPOSTGRES) $(LDFLAGS) -o $@

initdb: initdb.c $(LIBPOSTGRES)
	$(CC) $(CFLAGS) $< $(LIBPOSTGRES) $(LDFLAGS) -o $@

clean:
	rm -f $(EXAMPLES)
