include ../common.mk

CFLAGS += -I"../src" 
LIBPOSTGRES = ../src/libpostgres.a
CC = ~/Downloads/x86_64-linux-musl-native/bin/x86_64-linux-musl-gcc
#LDFLAGS += -lstdc++

EXAMPLES = example initdb reopen test_create_extension
EXTENSIONS_DIR = ../extension
EXTENSION_OBJS = $(EXTENSIONS_DIR)/example/example.a
EXTENSION_OBJS += $(EXTENSIONS_DIR)/plpgsql/libplpgsql_static.a
EXTENSION_OBJS += $(EXTENSIONS_DIR)/pgvector/libpgvector_static.a
EXTENSION_OBJS += $(EXTENSIONS_DIR)/postgis/libpostgis_static.a
#EXTENSION_OBJS += $(EXTENSIONS_DIR)/postgis/libpostgis_all.a
#EXTENSION_OBJS += ~/git/postgis/postgis-3.6.2dev/vendor/geos/build/lib/libgeos_c.a
#EXTENSION_OBJS += ~/git/postgis/postgis-3.6.2dev/vendor/geos/build/lib/libgeos.a
#EXTENSION_OBJS += ~/git/postgis/postgis-3.6.2dev/vendor/proj/build/lib/libproj.a
#EXTENSION_OBJS += ~/git/postgis/postgis-3.6.2dev/vendor/sqlite/libsqlite3.a
#EXTENSION_OBJS += ~/git/postgis/postgis-3.6.2dev/vendor/libxml2/.libs/libxml2.a

.PHONY: all clean extensions

all: $(EXAMPLES)

# Build extensions first
extensions:
	$(MAKE) -C .. extensions

# Pattern rule for simple examples
example initdb reopen: %: %.c $(LIBPOSTGRES) | extensions
	$(CC) $(CFLAGS) $< $(EXTENSION_OBJS) $(LIBPOSTGRES) $(LDFLAGS) -o $@

test_create_extension: test_create_extension.c $(EXTENSION_OBJS) $(LIBPOSTGRES) | extensions
	$(CC) $(CFLAGS) $< $(EXTENSION_OBJS) $(LIBPOSTGRES) $(LDFLAGS) -o $@

clean:
	rm -f $(EXAMPLES)
	$(MAKE) -C $(EXTENSIONS_DIR) clean
