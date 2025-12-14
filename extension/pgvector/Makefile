include ../../common.mk

CFLAGS += -I"../../src" -fPIC

LIBVECTOR = ../../vendor/pgvector/libvector.a

WRAPPER_SRC = pgvector_static.c
WRAPPER_OBJ = pgvector_static.o

ARCHIVE = libpgvector_static.a

.PHONY: all clean

all: $(ARCHIVE)

$(LIBVECTOR):
	bash build-static.sh

$(WRAPPER_SRC): $(LIBVECTOR)
	python3 ../../helpers/gen_static_extension.py pgvector $(LIBVECTOR) ../../vendor/pgvector/vector.control ../../vendor/pgvector/sql/vector.sql > $@

$(WRAPPER_OBJ): $(WRAPPER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCHIVE): $(WRAPPER_OBJ) $(LIBVECTOR)
	mkdir -p .tmp_objs
	cd .tmp_objs && $(AR) x ../$(LIBVECTOR)
	for obj in .tmp_objs/*.o; do \
		objcopy --localize-symbol=Pg_magic_func \
		        --redefine-sym=_PG_init=pgvector_PG_init $$obj; \
	done
	$(AR) rcs $@ $(WRAPPER_OBJ) .tmp_objs/*.o

clean:
	rm -f $(WRAPPER_SRC) $(WRAPPER_OBJ) $(ARCHIVE)
	rm -rf .tmp_objs
	rm -f extension/*
