include ../../common.mk

CFLAGS += -I"../../src" -I$(PG_INCLUDE) -fPIC

PLPGSQL_DIR = ../../vendor/pg18/src/pl/plpgsql
PLPGSQL_SRC_DIR = $(PLPGSQL_DIR)/src
PLPGSQL_OBJS = $(PLPGSQL_SRC_DIR)/pl_comp.o \
               $(PLPGSQL_SRC_DIR)/pl_exec.o \
               $(PLPGSQL_SRC_DIR)/pl_funcs.o \
               $(PLPGSQL_SRC_DIR)/pl_gram.o \
               $(PLPGSQL_SRC_DIR)/pl_handler.o \
               $(PLPGSQL_SRC_DIR)/pl_scanner.o

WRAPPER_SRC = plpgsql_static.c
WRAPPER_OBJ = plpgsql_static.o

ARCHIVE = libplpgsql_static.a

.PHONY: all clean

all: $(ARCHIVE)

$(PLPGSQL_SRC_DIR)/pl_handler.o:
	$(MAKE) -C $(PLPGSQL_DIR)

$(WRAPPER_SRC): $(PLPGSQL_SRC_DIR)/pl_handler.o
	python3 ../../helpers/gen_static_extension.py plpgsql $(PLPGSQL_SRC_DIR)/pl_handler.o $(PLPGSQL_SRC_DIR)/plpgsql.control $(PLPGSQL_SRC_DIR)/plpgsql--1.0.sql 1.0 > $@

$(WRAPPER_OBJ): $(WRAPPER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCHIVE): $(WRAPPER_OBJ) $(PLPGSQL_OBJS)
	mkdir -p .tmp_objs
	cp $(PLPGSQL_OBJS) .tmp_objs/
	for obj in .tmp_objs/*.o; do \
		objcopy --localize-symbol=Pg_magic_func \
		        --redefine-sym=_PG_init=plpgsql_PG_init $$obj; \
	done
	$(AR) rcs $@ $(WRAPPER_OBJ) .tmp_objs/*.o

clean:
	rm -f $(WRAPPER_OBJ) $(ARCHIVE) $(WRAPPER_SRC)
	rm -rf .tmp_objs
