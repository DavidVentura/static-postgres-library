#!/bin/bash
CC=musl-gcc \
CFLAGS="-O2 -g -static -ffunction-sections -fdata-sections" \
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
