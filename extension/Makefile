include ../common.mk

CFLAGS += -I"../src" -fPIC

EXTENSIONS = example_static.o

.PHONY: all clean

all: $(EXTENSIONS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	objcopy --redefine-sym=Pg_magic_func=example_static_Pg_magic_func \
	        --redefine-sym=_PG_init=example_static_PG_init $@

clean:
	rm -f $(EXTENSIONS)
