# Static Extensions

This directory contains PostgreSQL extensions that are statically linked into binaries instead of loaded dynamically.

## Structure

```
extensions/
├── Makefile                    # Builds extension .o files
├── example_static.c            # Example extension implementation
├── example_static.control      # Extension metadata
├── example_static--1.0.sql     # Extension SQL script
└── README.md                   # This file

examples/
├── Makefile                    # Links extensions into test programs
├── test_static_extension.c    # Test using CREATE FUNCTION
└── test_create_extension.c    # Test using CREATE EXTENSION
```

## Two Approaches

### Approach 1: CREATE FUNCTION (simple)
**Test**: `test_static_extension`

Manually create each function with CREATE FUNCTION:
```sql
CREATE FUNCTION add_one(integer) RETURNS integer
AS 'example_static', 'add_one' LANGUAGE C STRICT;
```

**Pros**: Simple, no extra files needed
**Cons**: No version management, must create each function individually

### Approach 2: CREATE EXTENSION (proper)
**Test**: `test_create_extension`

Use PostgreSQL's extension system:
```sql
CREATE EXTENSION example_static;
```

**Requires**:
- `.control` file (metadata)
- `--1.0.sql` file (CREATE FUNCTION statements)
- Files in `share/extension/` directory

**Pros**: Proper extension management, versioning, single command
**Cons**: Requires extra files and directory setup

## How It Works

1. **Extension Code** (`extensions/*.c`):
   - Implements PostgreSQL functions using PG_FUNCTION_INFO_V1
   - Defines a function table array
   - Provides a registration function

2. **Compilation** (`extensions/Makefile`):
   - Compiles extensions to `.o` files
   - Does NOT create shared libraries

3. **Linking** (`examples/Makefile`):
   - Links extension `.o` files into the final binary
   - Binary contains extension code statically

4. **Registration** (at runtime):
   - Test program calls `register_<extension>()` before pg_embedded_init()
   - Extension functions are registered with PostgreSQL
   - CREATE FUNCTION uses the library name to find statically-linked functions

## Adding a New Extension

### 1. Create Extension File

Create `extensions/myext.c`:

```c
#include "postgres.h"
#include "fmgr.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(my_func);
Datum
my_func(PG_FUNCTION_ARGS)
{
    // Implementation
}

void _PG_init(void)
{
    // Optional initialization
}

const StaticExtensionFunc myext_functions[] = {
    {"my_func", my_func, pg_finfo_my_func},
    {NULL, NULL, NULL}
};

void register_myext(void)
{
    register_static_extension(
        "myext",
        Pg_magic_func(),
        _PG_init,
        myext_functions
    );
}
```

### 2. Update extensions/Makefile

```makefile
EXTENSIONS = example_static.o myext.o
```

### 3. Update examples/Makefile

```makefile
EXTENSION_OBJS = $(EXTENSIONS_DIR)/example_static.o \
                 $(EXTENSIONS_DIR)/myext.o
```

### 4. Use in Test Program

```c
extern void register_myext(void);

int main() {
    register_myext();
    pg_embedded_init(...);

    // CREATE FUNCTION my_func() ...
    // AS 'myext', 'my_func' LANGUAGE C STRICT;
}
```

## Example: timestamp9

For a complex extension like timestamp9:

1. Create `extensions/timestamp9.c` with all timestamp9 functions
2. Build function table with all exported functions
3. Link into your embedded PostgreSQL binary
4. No .so file needed!

## Benefits

- **Single binary** - No separate .so files to deploy
- **No dlopen** - Faster, no dynamic loading overhead
- **Embedded systems** - Works without dynamic linker
- **Simplified deployment** - Just copy one binary
