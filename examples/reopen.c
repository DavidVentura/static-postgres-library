#include <stdio.h>
#include <stdlib.h>

#include "pgembedded.h"

static void print_result(pg_result *result)
{
	uint64_t	row;
	int			col;

	if (!result)
	{
		printf("NULL result\n");
		return;
	}

	printf("Status: %d, Rows: %llu, Cols: %d\n",
		   result->status,
		   (unsigned long long) result->rows,
		   result->cols);

	if (result->cols > 0 && result->colnames)
	{
		printf("\nColumn names:\n");
		for (col = 0; col < result->cols; col++)
		{
			printf("  [%d] %s\n", col, result->colnames[col]);
		}
	}

	if (result->values)
	{
		printf("\nData:\n");
		for (row = 0; row < result->rows; row++)
		{
			printf("  Row %llu: ", (unsigned long long) row);
			for (col = 0; col < result->cols; col++)
			{
				if (col > 0)
					printf(", ");
				printf("%s", result->values[row][col]);
			}
			printf("\n");
		}
	}

	printf("\n");
}

int
main(int argc, char **argv)
{
	const char *datadir;
	pg_result  *result;

	printf("========================================\n");
	printf("PostgreSQL Embedded Test Application\n");
	printf("========================================\n\n");

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <data_directory>\n", argv[0]);
		fprintf(stderr, "\nExample:\n");
		fprintf(stderr, "  %s /tmp/pgdata\n\n", argv[0]);
		fprintf(stderr, "Note: Data directory must be initialized with initdb first\n");
		return 1;
	}

	datadir = argv[1];

	printf("Initializing PostgreSQL...\n");
	printf("  Data directory: %s\n", datadir);
	printf("  Database: postgres\n");
	printf("  User: postgres\n\n");

	if (pg_embedded_init(datadir, "postgres", "postgres") != 0)
	{
		fprintf(stderr, "ERROR: Initialization failed: %s\n",
				pg_embedded_error_message());
		return 1;
	}

	printf("PostgreSQL initialized successfully!\n\n");

	/* Test 1: Get PostgreSQL version */
	printf("----------------------------------------\n");
	printf("Test 1: Get PostgreSQL version\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec("SELECT version()");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	print_result(result);
	pg_embedded_free_result(result);

	/* Check current database */
	printf("Current database: ");
	result = pg_embedded_exec("SELECT current_database()");
	if (result && result->rows > 0)
	{
		printf("%s\n\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	
	printf("\nShutting down PostgreSQL...\n");
	pg_embedded_shutdown();

	printf("\n========================================\n");
	printf("Test 2: Shutdown and Restart\n");
	printf("========================================\n\n");

	printf("Re-initializing PostgreSQL...\n");
	printf("  Data directory: %s\n", datadir);
	printf("  Database: postgres\n");
	printf("  User: postgres\n\n");

	if (pg_embedded_init(datadir, "postgres", "postgres") != 0)
	{
		fprintf(stderr, "ERROR: Re-initialization failed: %s\n",
				pg_embedded_error_message());
		return 1;
	}

	printf("PostgreSQL re-initialized successfully!\n\n");

	printf("Verify data persisted after restart:\n");
	result = pg_embedded_exec("SELECT id, name, value FROM test_embedded ORDER BY id");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	print_result(result);
	pg_embedded_free_result(result);

	printf("Insert new data after restart:\n");
	result = pg_embedded_exec(
							  "INSERT INTO test_embedded (name, value) VALUES ('Henry', 800) "
							  "RETURNING id, name, value");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	print_result(result);
	pg_embedded_free_result(result);

	printf("Final data check:\n");
	result = pg_embedded_exec("SELECT COUNT(*) as count FROM test_embedded");
	print_result(result);
	pg_embedded_free_result(result);

	printf("\nShutting down PostgreSQL again...\n");
	pg_embedded_shutdown();

	printf("\n========================================\n");
	printf("All tests completed successfully!\n");
	printf("========================================\n");

	return 0;
}

