#!/bin/bash
set -eu

VENDOR_DIR="$(realpath ../../vendor)"
POSTGIS_DIR="$VENDOR_DIR/postgis"
PG_INCLUDE="$VENDOR_DIR/pg18/src/include"
GEOS_DIR="$VENDOR_DIR/geos"
PROJ_DIR="$VENDOR_DIR/proj"
LIBXML2_DIR="$VENDOR_DIR/libxml2"

BUILD_DIR="$POSTGIS_DIR/build"
OUTPUT="$BUILD_DIR/libpostgis.a"

echo "Building PostGIS static library..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

GEOS_CAPI_INCLUDE="$GEOS_DIR/build/capi"
XML_INCLUDE="$LIBXML2_DIR/include"

INCLUDES="-I$PG_INCLUDE"
INCLUDES="$INCLUDES -I$POSTGIS_DIR/liblwgeom"
INCLUDES="$INCLUDES -I$POSTGIS_DIR/libpgcommon"
INCLUDES="$INCLUDES -I$POSTGIS_DIR/deps/ryu/.."
INCLUDES="$INCLUDES -I$POSTGIS_DIR/deps/flatgeobuf"
INCLUDES="$INCLUDES -I$POSTGIS_DIR/deps/flatgeobuf/include"
INCLUDES="$INCLUDES -I$POSTGIS_DIR"
INCLUDES="$INCLUDES -I$GEOS_CAPI_INCLUDE"
INCLUDES="$INCLUDES -I$XML_INCLUDE"

INCLUDES="$INCLUDES -I$GEOS_DIR/include"
INCLUDES="$INCLUDES -I$GEOS_DIR/build/include"
INCLUDES="$INCLUDES -I$PROJ_DIR/src"
INCLUDES="$INCLUDES -I$PROJ_DIR/include"
INCLUDES="$INCLUDES -I$PROJ_DIR/build/include"


DEFINES="-DHAVE_GEOS=1 -DHAVE_LIBPROJ=1 -DHAVE_LIBXML2=1"

for srcdir in "liblwgeom" "liblwgeom/topo" "libpgcommon" "deps/ryu" "postgis"; do
    prefix=$(basename "$srcdir")
    for src in $POSTGIS_DIR/$srcdir/*.c; do
        [ -f "$src" ] || continue
        basename=$(basename "$src")

        case "$basename" in
            lwgeom_sfcgal.c|vector_tile.pb-c.c|geobuf.pb-c.c)
                continue
                ;;
        esac

	out="${prefix}_$(basename "$src" .c).o"
        [ ! -f "$out" ] && $CC $CFLAGS $INCLUDES $DEFINES -c "$src" -o "$out"
    done
done

for src in $POSTGIS_DIR/deps/flatgeobuf/*.cpp; do
    [ -f "$src" ] || continue
    basename=$(basename "$src")
    $CXX $CXXFLAGS $INCLUDES -c "$src" -o "flatgeobuf_$(basename "$src" .cpp).o"
done

echo "Creating static library $OUTPUT..."
ar rcs "$OUTPUT" *.o

echo "PostGIS built: $(ls -lh $OUTPUT | awk '{print $9, $5}')"
