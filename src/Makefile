# Makefile for building libpostgres.a with embedded PostgreSQL
# This combines our custom C files with the PostgreSQL libraries

include ../common.mk

# Source files
SRCS = pgembedded.c initdb_simple.c stubs.c pg_reset.c pg_notification.c extensions.c embedded_fopen.c embedded_timezone.c
OBJS = $(SRCS:.c=.o)

GENERATED = embedded_timezone_data.h

OUTPUT = libpostgres.a

TZ_DEFAULT_FILE = ../vendor/pg18/src/timezone/tznames/Default

.PHONY: all clean

all: $(GENERATED) $(OUTPUT)

embedded_timezone_data.h: $(TZ_DEFAULT_FILE)
	xxd -include -name timezone_default < $< | sed 's/unsigned/const unsigned/' > $@

# Compile C files to object files
%.o: %.c
	$(CC) $(CFLAGS) -I$(PG_INCLUDE) -c $< -o $@

# Build final static library using MRI script
$(OUTPUT): $(OBJS) 
	bash pack_archive.sh "$(PG_BACKEND_LIBS)" "$@" $(OBJS)
	@echo "Object count: $$(ar t $@ | wc -l)"

clean:
	rm -f $(OBJS) $(OUTPUT) libpostgres.mri $(GENERATED)
