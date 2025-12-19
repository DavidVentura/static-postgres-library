#!/bin/bash
set -eu

cd ../../vendor/pgvector/
SOURCES=(src/*.c)

for src in "${SOURCES[@]}"; do
    obj="${src%.c}.o"
    $CC $CFLAGS -c "$src" -o "$obj" &
done
wait

ar rcs "$OUTPUT_LIB" src/*.o
