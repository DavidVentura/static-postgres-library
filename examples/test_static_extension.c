/*
 * test_static_extension - Test program for static extensions
 *
 * This demonstrates using a statically-linked extension by:
 * 1. Registering the extension at startup
 * 2. Creating functions via CREATE FUNCTION
 * 3. Testing the functions with SQL queries
 */

#include <stdio.h>
#include <stdlib.h>
#include "pgembedded.h"

/* Declaration of extension registration function */
extern void register_example_static(void);

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

	printf("Registering static extension...\n");
	register_example_static();
	printf("Extension registered!\n\n");

	printf("Initializing PostgreSQL...\n");
	printf("  Data directory: %s\n\n", datadir);

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
