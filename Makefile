.PHONY: all build clean src examples patch pg-backend-libs extensions

include ./common.mk

# Default target builds everything
all: build examples

patch:
	cd vendor/pg18 && find ../../patches/pg18 -type f -name '*.patch' | while read -r line; do patch -p1 < $$line; done
	cd vendor/proj && find ../../patches/proj -type f -name '*.patch' | while read -r line; do patch -p1 < $$line; done

# Build the base PostgreSQL components and our library
build: src

extensions: pg-backend-libs
	$(MAKE) -C extension

# Build examples (depends on src being built)
examples: src extensions
	$(MAKE) -C examples

# Configure PostgreSQL
pg-configure: vendor/pg18/src/Makefile.global

vendor/pg18/src/Makefile.global:
	cd vendor/pg18 && \
	CC=$(CC) \
	CFLAGS="$(CFLAGS) -Datexit=__wrap_atexit " \
	LDFLAGS="$(LDFLAGS) -static" \
	./configure \
	  --prefix=/tmp/pg-embedded-install \
	  --without-readline \
	  --without-zlib \
	  --without-icu \
	  --with-openssl=no \
	  --without-ldap \
	  --without-pam \
	  --without-gssapi \
	  --without-systemd \
	  --without-llvm \
	  --disable-largefile \
	  --disable-debug

# Build PostgreSQL backend object files and libraries
pg-backend-libs: pg-configure
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
	rm -f vendor/pg18/src/Makefile.global
	rm -f ./vendor/pg18/src/backend/backend-libs.txt
