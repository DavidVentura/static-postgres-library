CC = musl-gcc
CFLAGS = -static -I"../src" -I"../vendor/pg18/src/include" -O0 -ggdb
LDFLAGS = -Wl,--gc-sections
LIBPOSTGRES = ../src/libpostgres.a
EXTENSIONS_DIR = ../extension

EXAMPLES = example initdb reopen test_static_extension test_create_extension
EXTENSION_OBJS = $(EXTENSIONS_DIR)/example_static.o $(EXTENSIONS_DIR)/plpgsql/libplpgsql_static.a

.PHONY: all clean extensions

all: $(EXAMPLES)

# Build extensions first
extensions:
	$(MAKE) -C $(EXTENSIONS_DIR)

# Pattern rule for simple examples
example initdb reopen: %: %.c $(LIBPOSTGRES)
	$(CC) $(CFLAGS) $< $(LIBPOSTGRES) $(LDFLAGS) -o $@

# Special rules for static extension tests - link with extension object
test_static_extension: test_static_extension.c $(EXTENSION_OBJS) $(LIBPOSTGRES) | extensions
	$(CC) $(CFLAGS) $< $(EXTENSION_OBJS) $(LIBPOSTGRES) $(LDFLAGS) -o $@

test_create_extension: test_create_extension.c $(EXTENSION_OBJS) $(LIBPOSTGRES) | extensions
	$(CC) $(CFLAGS) $< $(EXTENSION_OBJS) $(LIBPOSTGRES) $(LDFLAGS) -o $@

clean:
	rm -f $(EXAMPLES)
	$(MAKE) -C $(EXTENSIONS_DIR) clean
