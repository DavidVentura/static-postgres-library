#!/bin/bash
set -e

# Configuration
CC="${CC:-musl-gcc}"
CFLAGS="${CFLAGS:--static -fPIC}"
INCLUDE_DIR="${INCLUDE_DIR:-../pg18/src/include}"
OUTPUT_LIB="${OUTPUT_LIB:-libvector.a}"

# from original Makefile
OPTFLAGS="-march=native -ftree-vectorize -fassociative-math -fno-signed-zeros -fno-trapping-math"
FULL_CFLAGS="$CFLAGS -I$INCLUDE_DIR $OPTFLAGS"

SOURCES=(src/*.c)

cd ../../vendor/pgvector/

echo "Cleaning old object files..."
rm -f src/*.o

for src in "${SOURCES[@]}"; do
    obj="${src%.c}.o"
    $CC $FULL_CFLAGS -c "$src" -o "$obj" &
done
wait

ar rcs "$OUTPUT_LIB" src/*.o
ar t "$OUTPUT_LIB"
