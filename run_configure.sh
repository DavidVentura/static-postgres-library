#!/bin/bash
cd vendor/pg18
CC=musl-gcc \
CFLAGS="-O0 -ggdb -static -ffunction-sections -fdata-sections -Datexit=__wrap_atexit " \
LDFLAGS="-static" \
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
