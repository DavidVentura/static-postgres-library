#!/bin/bash
set -e

# Configuration
CC="${CC:-musl-gcc}"
CFLAGS="${CFLAGS:--static -fPIC}"
INCLUDE_DIR="${INCLUDE_DIR:-../pg18/src/include}"
OUTPUT_LIB="${OUTPUT_LIB:-libvector.a}"

# Additional optimization flags from original Makefile
OPTFLAGS="-march=native -ftree-vectorize -fassociative-math -fno-signed-zeros -fno-trapping-math"

# Full compilation flags
FULL_CFLAGS="$CFLAGS -I$INCLUDE_DIR $OPTFLAGS"

# Source files
SOURCES=(
    src/bitutils.c
    src/bitvec.c
    src/halfutils.c
    src/halfvec.c
    src/hnsw.c
    src/hnswbuild.c
    src/hnswinsert.c
    src/hnswscan.c
    src/hnswutils.c
    src/hnswvacuum.c
    src/ivfbuild.c
    src/ivfflat.c
    src/ivfinsert.c
    src/ivfkmeans.c
    src/ivfscan.c
    src/ivfutils.c
    src/ivfvacuum.c
    src/sparsevec.c
    src/vector.c
)

cd ../../vendor/pgvector/

# Clean old object files
echo "Cleaning old object files..."
rm -f src/*.o

# Compile each source file
echo "Compiling with: $CC"
echo "CFLAGS: $FULL_CFLAGS"
echo ""

for src in "${SOURCES[@]}"; do
    obj="${src%.c}.o"
    echo "Compiling $src -> $obj"
    $CC $FULL_CFLAGS -c "$src" -o "$obj" &
done
wait

# Create static library
echo ""
echo "Creating static library: $OUTPUT_LIB"
ar rcs "$OUTPUT_LIB" src/*.o

echo ""
echo "Done! Static library created: $OUTPUT_LIB"
echo "Object files: $(ls -1 src/*.o | wc -l)"
ar t "$OUTPUT_LIB"
