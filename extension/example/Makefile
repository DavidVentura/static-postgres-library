include ../../common.mk

CFLAGS += -I"../../src" -I$(PG_INCLUDE) -fPIC

WRAPPER_SRC = example_static.c
WRAPPER_OBJ = example_static.o
OBJ = example.o
ARCHIVE = example.a

.PHONY: all clean

$(ARCHIVE): $(WRAPPER_OBJ) $(OBJ)
	mkdir -p .tmp_objs
	cp $(OBJ) .tmp_objs
	for obj in .tmp_objs/*.o; do \
		objcopy --localize-symbol=Pg_magic_func \
		        --redefine-sym=_PG_init=example_PG_init $$obj; \
	done
	$(AR) rcs $@ $(WRAPPER_OBJ) .tmp_objs/*.o

$(WRAPPER_OBJ): $(WRAPPER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(WRAPPER_SRC): $(OBJ)
	python3 ../../helpers/gen_static_extension.py example $(OBJ) example_static.control example_static--1.0.sql 1.0 > $@

clean:
	rm -f $(WRAPPER_SRC) $(WRAPPER_OBJ) $(OBJ) $(ARCHIVE)
