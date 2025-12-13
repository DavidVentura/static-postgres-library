# Makefile for building libpostgres.a with embedded PostgreSQL
# This combines our custom C files with the PostgreSQL libraries

# Configuration
CC = musl-gcc
CFLAGS = -static -I"../vendor/pg18/src/include" -O0 -ggdb -fdata-sections -ffunction-sections
LDFLAGS = -Wl,--gc-sections

# PostgreSQL backend libs list
PG_BACKEND_LIBS = ../vendor/pg18/src/backend/backend-libs.txt

# Source files
SRCS = pgembedded.c initdb_simple.c stubs.c pg_reset.c pg_notification.c
OBJS = $(SRCS:.c=.o)

OUTPUT = libpostgres.a

.PHONY: all clean

all: $(OUTPUT)

# Compile C files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build final static library using MRI script
$(OUTPUT): $(OBJS) 
	bash pack_archive.sh "$(PG_BACKEND_LIBS)" "$@" $(OBJS)
	@echo "Object count: $$(ar t $@ | wc -l)"

clean:
	rm -f $(OBJS) $(OUTPUT) libpostgres.mri
