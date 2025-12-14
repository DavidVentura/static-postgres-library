include ../common.mk

CFLAGS += -I"../src"
LIBPOSTGRES = ../src/libpostgres.a
EXTENSIONS_DIR = ../extension

EXAMPLES = example initdb reopen test_create_extension
EXTENSION_OBJS = $(EXTENSIONS_DIR)/example_static.o
EXTENSION_OBJS += $(EXTENSIONS_DIR)/plpgsql/libplpgsql_static.a
EXTENSION_OBJS += $(EXTENSIONS_DIR)/pgvector/libpgvector_static.a

.PHONY: all clean extensions

all: $(EXAMPLES)

# Build extensions first
extensions:
	$(MAKE) -C $(EXTENSIONS_DIR)

# Pattern rule for simple examples
example initdb reopen: %: %.c $(LIBPOSTGRES)
	$(CC) $(CFLAGS) $< $(LIBPOSTGRES) $(LDFLAGS) -o $@

test_create_extension: test_create_extension.c $(EXTENSION_OBJS) $(LIBPOSTGRES) | extensions
	$(CC) $(CFLAGS) $< $(EXTENSION_OBJS) $(LIBPOSTGRES) $(LDFLAGS) -o $@

clean:
	rm -f $(EXAMPLES)
	$(MAKE) -C $(EXTENSIONS_DIR) clean
