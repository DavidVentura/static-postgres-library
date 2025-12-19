include ../../common.mk

CFLAGS += -fPIC

WRAPPER_SRC = postgis_static.c
WRAPPER_OBJ = postgis_static.o

INPUT_A = libpostgis_all.a
ARCHIVE = libpostgis_static.a

# Vendor directories
VENDOR_DIR = ../../vendor
SQLITE_DIR = $(VENDOR_DIR)/sqlite
LIBXML2_DIR = $(VENDOR_DIR)/libxml2
GEOS_DIR = $(VENDOR_DIR)/geos
PROJ_DIR = $(VENDOR_DIR)/proj
POSTGIS_DIR = $(VENDOR_DIR)/postgis
PG_INCLUDE = $(VENDOR_DIR)/pg18/src/include

# Dependency libraries
SQLITE_LIB = $(SQLITE_DIR)/libsqlite3.a
LIBXML2_LIB = $(LIBXML2_DIR)/.libs/libxml2.a
GEOS_LIB = $(GEOS_DIR)/build/lib/libgeos.a
GEOS_C_LIB = $(GEOS_DIR)/build/lib/libgeos_c.a
PROJ_LIB = $(PROJ_DIR)/build/lib/libproj.a
POSTGIS_LIB = $(POSTGIS_DIR)/build/libpostgis.a
POSTGIS_CONFIG = $(POSTGIS_DIR)/postgis_config.h
LIBLWGEOM_H = $(POSTGIS_DIR)/liblwgeom/liblwgeom.h

.PHONY: all clean clean-deps

all: $(ARCHIVE)

# SQLite build target
$(SQLITE_LIB):
	@echo "Building SQLite3..."
	cd $(SQLITE_DIR) && \
	$(CC) -O2 -fPIC -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o sqlite3.o && \
	ar rcs libsqlite3.a sqlite3.o

# libxml2 build target
$(LIBXML2_LIB):
	@echo "Building libxml2..."
	cd $(LIBXML2_DIR) && \
	./autogen.sh --prefix=/usr --disable-shared --enable-static --with-pic --without-python --without-lzma --without-zlib && \
	$(MAKE) libxml2.la

# GEOS build target
$(GEOS_LIB):
	@echo "Building GEOS..."
	cd $(GEOS_DIR) && \
	mkdir -p build && \
	cd build && \
	cmake -DCMAKE_BUILD_TYPE=Release \
	      -DBUILD_SHARED_LIBS=OFF \
	      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	      -DBUILD_TESTING=OFF \
	      -DBUILD_GEOSOP=OFF \
	      -DCMAKE_C_COMPILER=$(CC) \
	      -DCMAKE_CXX_COMPILER=$(CXX) \
	      .. && \
	$(MAKE)

# PROJ build target (depends on SQLite)
$(PROJ_LIB): $(SQLITE_LIB)
	@echo "Building PROJ..."
	cd $(PROJ_DIR) && \
	mkdir -p build && \
	cd build && \
	cmake -DCMAKE_BUILD_TYPE=Release \
	      -DBUILD_SHARED_LIBS=OFF \
	      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	      -DENABLE_TIFF=OFF \
	      -DENABLE_CURL=OFF \
	      -DBUILD_TESTING=OFF \
	      -DBUILD_APPS=OFF \
	      -DSQLite3_INCLUDE_DIR=$(abspath $(SQLITE_DIR)) \
	      -DSQLite3_LIBRARY=$(abspath $(SQLITE_LIB)) \
	      .. && \
	$(MAKE)

# PostGIS configure target - generate minimal config
$(POSTGIS_CONFIG): $(GEOS_LIB) $(PROJ_LIB) $(LIBXML2_LIB)
	@echo "Generating PostGIS config..."
	echo '#define HAVE_GEOS 1' > $@
	echo '#define HAVE_LIBXML2 1' >> $@
	echo '#define HAVE_PROJ 1' >> $@
	echo '#define POSTGIS_GEOS_VERSION 31202' >> $@
	echo '#define POSTGIS_PROJ_VERSION 95' >> $@
	echo '#define POSTGIS_VERSION "3.6.1"' >> $@
	echo '#define POSTGIS_LIB_VERSION "3.6.1"' >> $@
	echo '#define POSTGIS_MAJOR_VERSION "3"' >> $@
	echo '#define POSTGIS_MINOR_VERSION "6"' >> $@
	echo '#define POSTGIS_DEBUG_LEVEL 0' >> $@
	echo '#define POSTGIS_PGSQL_VERSION 180' >> $@
	echo '#define POSTGIS_BUILD_DATE "2024-01-01"' >> $@
	echo '#define POSTGIS_LIBXML2_VERSION "2.13.5"' >> $@

# Generate liblwgeom.h from template
$(LIBLWGEOM_H): $(POSTGIS_DIR)/liblwgeom/liblwgeom.h.in
	@echo "Generating liblwgeom.h..."
	sed -e 's/@POSTGIS_LIB_VERSION@/3.6.1/g' \
	    -e 's/@POSTGIS_MAJOR_VERSION@/3/g' \
	    -e 's/@POSTGIS_MINOR_VERSION@/6/g' \
	    -e 's/@POSTGIS_GEOS_VERSION@/31202/g' \
	    -e 's/@SRID_MAX@/999999/g' \
	    -e 's/@SRID_USR_MAX@/998999/g' \
	    $< > $@

# PostGIS build target (depends on all dependencies)
$(POSTGIS_LIB): $(POSTGIS_CONFIG) $(LIBLWGEOM_H)
	CFLAGS='$(CFLAGS)' CC="$(CC)" CXX="$(CXX)" ./build_postgis.sh

# Merge all libraries into INPUT_A
$(INPUT_A): $(POSTGIS_LIB)
	@echo "Merging all dependencies into $@..."
	printf '%s\n' \
		'CREATE $@' \
		'ADDLIB $(POSTGIS_LIB)' \
		'ADDLIB $(GEOS_C_LIB)' \
		'ADDLIB $(GEOS_LIB)' \
		'ADDLIB $(PROJ_LIB)' \
		'ADDLIB $(SQLITE_LIB)' \
		'ADDLIB $(LIBXML2_LIB)' \
		'SAVE' \
		'END' \
		| ar -M
	 

proj_db.h: proj.db
	xxd -i proj.db | sed 's/unsigned/const unsigned static/' > $@

$(WRAPPER_SRC): $(INPUT_A)
	python3 ../../helpers/gen_static_extension.py postgis $(INPUT_A) postgis.control postgis--3.6.2dev.sql 3.6.2dev > $@

embedded_proj_db.o: embedded_proj_db.c proj_db.h
	$(CC) -I"$(SQLITE_DIR)" $(CFLAGS) -c $< -o $@

$(WRAPPER_OBJ): $(WRAPPER_SRC) proj_db.h embedded_proj_db.c
	$(CC) $(CFLAGS) -I../../src -c $< -o $@

$(ARCHIVE): $(WRAPPER_OBJ) embedded_proj_db.o
	cp $(INPUT_A) $@
	objcopy --localize-symbol=Pg_magic_func \
		--redefine-sym=_PG_init=postgis_PG_init $@
	$(AR) rs $@ $(WRAPPER_OBJ) embedded_proj_db.o

clean:
	rm -f $(WRAPPER_OBJ) $(ARCHIVE) $(WRAPPER_SRC) embedded_proj_db.o proj_db.h $(POSTGIS_LIB) $(INPUT_A)
	rm -rf $(POSTGIS_DIR)/build

clean-deps:
	rm -rf $(SQLITE_DIR)/*.o $(SQLITE_DIR)/*.a
	cd $(LIBXML2_DIR) && git clean -fd
	cd $(GEOS_DIR) && git clean -fd
	cd $(PROJ_DIR) && git clean -fd
	cd $(POSTGIS_DIR) && git clean -fd
