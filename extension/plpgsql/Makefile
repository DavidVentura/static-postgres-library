include ../../common.mk

CFLAGS += -I"../../src" -fPIC

PLPGSQL_SRC_DIR = ../../vendor/pg18/src/pl/plpgsql/src
PLPGSQL_OBJS = $(PLPGSQL_SRC_DIR)/pl_comp.o \
               $(PLPGSQL_SRC_DIR)/pl_exec.o \
               $(PLPGSQL_SRC_DIR)/pl_funcs.o \
               $(PLPGSQL_SRC_DIR)/pl_gram.o \
               $(PLPGSQL_SRC_DIR)/pl_handler.o \
               $(PLPGSQL_SRC_DIR)/pl_scanner.o

WRAPPER_OBJ = plpgsql_static.o

ARCHIVE = libplpgsql_static.a

.PHONY: all clean

all: $(ARCHIVE)

$(PLPGSQL_SRC_DIR)/pl_handler.o:
	cd ../../vendor/pg18/src/pl/plpgsql/ && make

$(WRAPPER_OBJ): plpgsql_static.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCHIVE): $(WRAPPER_OBJ) $(PLPGSQL_OBJS)
	mkdir -p .tmp_objs
	cp $(PLPGSQL_OBJS) .tmp_objs/
	for obj in .tmp_objs/*.o; do \
		objcopy --redefine-sym=Pg_magic_func=plpgsql_Pg_magic_func \
		        --redefine-sym=_PG_init=plpgsql_PG_init $$obj; \
	done
	$(AR) rcs $@ $(WRAPPER_OBJ) .tmp_objs/*.o

clean:
	rm -f $(WRAPPER_OBJ) $(ARCHIVE)
	rm -rf .tmp_objs
