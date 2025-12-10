.PHONY: all build clean src examples patch pg-backend-libs

# Default target builds everything
all: build examples

patch:
	cd vendor/pg18 && patch -p1 < ../../patches/async.h.patch
	cd vendor/pg18 && patch -p1 < ../../patches/async.c.patch
	cd vendor/pg18 && patch -p1 < ../../patches/Makefile.patch

# Build the base PostgreSQL components and our library
build: src

# Build examples (depends on src being built)
examples: src
	$(MAKE) -C examples

# Build PostgreSQL backend object files and libraries
pg-backend-libs:
	$(MAKE) -j18 -C vendor/pg18/src/backend generated-headers submake-libpgport
	$(MAKE) -j18 -C vendor/pg18/src/backend backend-libs.txt

# Build our custom library in src/
src: pg-backend-libs
	$(MAKE) -C src

# Clean everything
clean:
	$(MAKE) -C vendor/pg18 clean || true
	$(MAKE) -C src clean || true
	$(MAKE) -C examples clean || true
	rm -f ./vendor/pg18/src/backend/backend-libs.txt
