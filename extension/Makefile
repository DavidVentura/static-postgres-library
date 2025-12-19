.PHONY: build clean example pgvector plpgsql

build: example pgvector plpgsql
	:

example:
	$(MAKE) -C example
pgvector:
	$(MAKE) -C pgvector
plpgsql:
	$(MAKE) -C plpgsql

clean:
	$(MAKE) -C example clean
	$(MAKE) -C pgvector clean
	$(MAKE) -C plpgsql clean
