include ../../common.mk

CFLAGS += -I"../../src" -I$(PG_INCLUDE) -fPIC

LIBVECTOR = ../../vendor/pgvector/libvector.a

WRAPPER_SRC = pgvector_static.c
WRAPPER_OBJ = pgvector_static.o

ARCHIVE = libpgvector_static.a

.PHONY: all clean

all: $(ARCHIVE)

$(LIBVECTOR):
	CC=$(CC) CFLAGS="$(CFLAGS)" OUTPUT_LIB=$@ bash build-static.sh

$(WRAPPER_SRC): $(LIBVECTOR)
	python3 ../../helpers/gen_static_extension.py vector $(LIBVECTOR) ../../vendor/pgvector/vector.control ../../vendor/pgvector/sql/vector.sql 0.8.1 > $@

$(WRAPPER_OBJ): $(WRAPPER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCHIVE): $(WRAPPER_OBJ) $(LIBVECTOR)
	mkdir -p .tmp_objs
	cd .tmp_objs && $(AR) x ../$(LIBVECTOR)
	for obj in .tmp_objs/*.o; do \
		objcopy --localize-symbol=Pg_magic_func \
		        --redefine-sym=_PG_init=vector_PG_init $$obj; \
	done
	$(AR) rcs $@ $(WRAPPER_OBJ) .tmp_objs/*.o

clean:
	rm -f $(WRAPPER_SRC) $(WRAPPER_OBJ) $(ARCHIVE)
	rm -rf .tmp_objs
	rm -f ../../vendor/pgvector/src/*.o
	rm -f $(LIBVECTOR)
	rm -f $(ARCHIVE)
