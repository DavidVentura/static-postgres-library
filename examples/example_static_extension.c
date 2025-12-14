/*
 * Example static extension for PostgreSQL
 *
 * This demonstrates how to create a statically-linked extension
 * using the register_static_extension() API.
 *
 * To use:
 * 1. Compile this file and link it into the postgres binary
 * 2. The extension will auto-register via __attribute__((constructor))
 * 3. In postgres, you can use: CREATE EXTENSION example_static;
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

#include <stdio.h>
#include <stdlib.h>
#include "pgembedded.h"

/* Declare the magic block */
PG_MODULE_MAGIC;

/*
 * Example function: add_one
 * Returns input + 1
 */
PG_FUNCTION_INFO_V1(add_one);

Datum
add_one(PG_FUNCTION_ARGS)
{
	int32 arg = PG_GETARG_INT32(0);
	PG_RETURN_INT32(arg + 1);
}

/*
 * Example function: hello_world
 * Returns a greeting string
 */
PG_FUNCTION_INFO_V1(hello_world);

Datum
hello_world(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P(cstring_to_text("Hello from static extension!"));
}

/*
 * Optional: _PG_init function called when library is loaded
 */
void
_PG_init(void)
{
	elog(NOTICE, "Example static extension initialized");
}

const StaticExtensionFunc funcs[3] = {
	{"add_one", add_one, pg_finfo_add_one},
	{"hello_world", hello_world, pg_finfo_hello_world},
	{NULL, NULL, NULL}  /* Terminator */
};
static void register_example_extension(void)
{
	/*
	 * Register this extension with PostgreSQL's static extension system
	 * Using compound literal to initialize function table at runtime
	 */
	register_static_extension(
		"example_static",
		Pg_magic_func(),
		_PG_init,
		funcs
	);
}

/*
 * Test main function - demonstrates the static extension
 */
int
main(int argc, char *argv[])
{
	const char *datadir;
	pg_result *result;

	printf("========================================\n");
	printf("Static Extension Test\n");
	printf("========================================\n\n");

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <data_directory>\n", argv[0]);
		fprintf(stderr, "\nExample:\n");
		fprintf(stderr, "  %s db_data\n\n", argv[0]);
		return 1;
	}

	datadir = argv[1];

	printf("Initializing PostgreSQL...\n");
	printf("  Data directory: %s\n\n", datadir);

	register_example_extension();

	if (pg_embedded_init(datadir, "postgres", "postgres") != 0)
	{
		fprintf(stderr, "ERROR: Initialization failed: %s\n",
				pg_embedded_error_message());
		return 1;
	}


	printf("PostgreSQL initialized successfully!\n\n");

	/* Create the add_one function */
	printf("----------------------------------------\n");
	printf("Test 1: Create add_one function\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec(
		"CREATE OR REPLACE FUNCTION add_one(integer) RETURNS integer "
		"AS 'example_static', 'add_one' "
		"LANGUAGE C STRICT");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
		pg_embedded_free_result(result);
		pg_embedded_shutdown();
		return 1;
	}
	printf("Function created successfully!\n\n");
	pg_embedded_free_result(result);

	/* Test add_one function */
	printf("----------------------------------------\n");
	printf("Test 2: Call add_one(41)\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec("SELECT add_one(41)");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("Result: %s\n", result->values[0][0]);
		printf("Expected: 42\n\n");
	}
	pg_embedded_free_result(result);

	/* Create the hello_world function */
	printf("----------------------------------------\n");
	printf("Test 3: Create hello_world function\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec(
		"CREATE OR REPLACE FUNCTION hello_world() RETURNS text "
		"AS 'example_static', 'hello_world' "
		"LANGUAGE C STRICT");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
		pg_embedded_free_result(result);
		pg_embedded_shutdown();
		return 1;
	}
	printf("Function created successfully!\n\n");
	pg_embedded_free_result(result);

	/* Test hello_world function */
	printf("----------------------------------------\n");
	printf("Test 4: Call hello_world()\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec("SELECT hello_world()");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("Result: %s\n\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	/* Cleanup */
	printf("----------------------------------------\n");
	printf("Shutting down\n");
	printf("----------------------------------------\n");
	pg_embedded_shutdown();

	printf("\n========================================\n");
	printf("All tests completed successfully!\n");
	printf("========================================\n");

	return 0;
}
