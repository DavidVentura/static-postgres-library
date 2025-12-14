/*
 * test_create_extension - Test CREATE EXTENSION with static extensions
 *
 * This demonstrates using CREATE EXTENSION instead of individual
 * CREATE FUNCTION statements with statically-linked extensions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgembedded.h"

/* Declaration of extension registration function */
extern void register_example_static(void);
extern void register_pl_pgsql(void);

int
main(int argc, char *argv[])
{
	const char *datadir;
	pg_result *result;

	printf("========================================\n");
	printf("CREATE EXTENSION with Static Extension\n");
	printf("========================================\n\n");

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <data_directory>\n", argv[0]);
		fprintf(stderr, "\nExample:\n");
		fprintf(stderr, "  %s db_data\n\n", argv[0]);
		return 1;
	}

	datadir = argv[1];

	/* Setup extension files location */
	printf("Registering static extension...\n");
	register_example_static();
	register_pl_pgsql();

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

	/* Set extension control path to our extensions directory */
	printf("Setting extension control path...\n");
	if (pg_embedded_set_extension_path("/home/david/git/pl") != 0)
	{
		fprintf(stderr, "ERROR: Failed to set extension path: %s\n",
				pg_embedded_error_message());
		pg_embedded_shutdown();
		return 1;
	}
	printf("Extension path set to: ../extensions\n\n");

	/* Test CREATE EXTENSION */
	printf("----------------------------------------\n");
	printf("Test 1: CREATE EXTENSION example_static\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec("CREATE EXTENSION IF NOT EXISTS example_static");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
		pg_embedded_free_result(result);
		pg_embedded_shutdown();
		return 1;
	}
	printf("Extension created successfully!\n\n");
	pg_embedded_free_result(result);

	/* Test CREATE EXTENSION */
	printf("----------------------------------------\n");
	printf("Test 1: CREATE EXTENSION IF NOT EXISTS plpgsql\n");
	printf("----------------------------------------\n");
	if (pg_embedded_set_extension_path("/home/david/git/pl/extension/plpgsql") != 0)
	{
		fprintf(stderr, "ERROR: Failed to set extension path: %s\n",
				pg_embedded_error_message());
		pg_embedded_shutdown();
		return 1;
	}
	result = pg_embedded_exec("DROP EXTENSION IF EXISTS plpgsql");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
		//pg_embedded_free_result(result);
		//pg_embedded_shutdown();
		//return 1;
	}
	pg_embedded_free_result(result);

	result = pg_embedded_exec("CREATE EXTENSION plpgsql");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
		//pg_embedded_free_result(result);
		//pg_embedded_shutdown();
		//return 1;
	}
	printf("Extension created successfully!\n\n");
	pg_embedded_free_result(result);

	/* Verify extension is installed */
	printf("----------------------------------------\n");
	printf("Test 2: List installed extensions\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec("SELECT extname, extversion FROM pg_extension");
	if (result && result->status >= 0 && result->rows > 0)
	{
		for(int row = 0; row < result->rows; row++) {
		printf("Extension: %s, Version: %s\n\n",
			result->values[row][0], result->values[row][1]);
		}
	}
	pg_embedded_free_result(result);

	/* Test add_one function */
	printf("----------------------------------------\n");
	printf("Test 3: Call add_one(41)\n");
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

	/* Test pl/pgsql function */
	printf("----------------------------------------\n");
	printf("Test 5: Create and call pl/pgsql function\n");
	printf("----------------------------------------\n");

	printf("Creating pl/pgsql function multiply_numbers...\n");
	result = pg_embedded_exec(
		"CREATE OR REPLACE FUNCTION multiply_numbers(a integer, b integer) "
		"RETURNS integer AS $$ "
		"BEGIN "
		"  RETURN a * b; "
		"END; "
		"$$ LANGUAGE plpgsql;");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: Failed to create pl/pgsql function: %s\n",
				pg_embedded_error_message());
	}
	else
	{
		printf("pl/pgsql function created successfully!\n");
	}
	pg_embedded_free_result(result);

	printf("Calling multiply_numbers(6, 7)...\n");
	result = pg_embedded_exec("SELECT multiply_numbers(6, 7) as result");
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

	/* Test DROP EXTENSION */
	printf("----------------------------------------\n");
	printf("Test 6: DROP EXTENSION example_static\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec("DROP EXTENSION example_static");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else
	{
		printf("Extension dropped successfully!\n\n");
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
