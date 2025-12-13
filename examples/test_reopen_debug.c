#include <stdio.h>
#include <stdlib.h>
#include "pgembedded.h"

static void print_result(pg_result *result)
{
	uint64_t row;
	int col;
	
	if (!result) {
		printf("NULL result\n");
		return;
	}
	
	printf("Status: %d, Rows: %llu, Cols: %d\n",
		   result->status,
		   (unsigned long long) result->rows,
		   result->cols);
	
	if (result->values) {
		for (row = 0; row < result->rows; row++) {
			printf("  Row %llu: ", (unsigned long long) row);
			for (col = 0; col < result->cols; col++) {
				if (col > 0) printf(", ");
				printf("%s", result->values[row][col]);
			}
			printf("\n");
		}
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	pg_result *result;
	
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <data_directory>\n", argv[0]);
		return 1;
	}
	
	printf("First init...\n");
	if (pg_embedded_init(argv[1], "postgres", "postgres") != 0) {
		fprintf(stderr, "ERROR: Init failed: %s\n", pg_embedded_error_message());
		return 1;
	}
	
	printf("\n=== Checking catalog tables ===\n");
	result = pg_embedded_exec("SELECT count(*) FROM pg_class WHERE relname = 'test_embedded'");
	printf("pg_class lookup for test_embedded: ");
	print_result(result);
	pg_embedded_free_result(result);
	
	result = pg_embedded_exec("SELECT nspname FROM pg_namespace ORDER BY nspname");
	printf("Available schemas: ");
	print_result(result);
	pg_embedded_free_result(result);
	
	result = pg_embedded_exec("SHOW search_path");
	printf("Current search_path: ");
	print_result(result);
	pg_embedded_free_result(result);
	
	result = pg_embedded_exec("SELECT relname, relnamespace::regnamespace FROM pg_class WHERE relkind = 'r' AND relnamespace NOT IN (SELECT oid FROM pg_namespace WHERE nspname LIKE 'pg_%' OR nspname = 'information_schema')");
	printf("User tables: ");
	print_result(result);
	pg_embedded_free_result(result);
	
	printf("\nShutting down...\n");
	pg_embedded_shutdown();
	
	printf("\n=== REOPEN ===\n");
	if (pg_embedded_init(argv[1], "postgres", "postgres") != 0) {
		fprintf(stderr, "ERROR: Re-init failed: %s\n", pg_embedded_error_message());
		return 1;
	}
	
	printf("\n=== After reopen - checking catalog tables ===\n");
	result = pg_embedded_exec("SELECT count(*) FROM pg_class WHERE relname = 'test_embedded'");
	printf("pg_class lookup for test_embedded: ");
	print_result(result);
	pg_embedded_free_result(result);
	
	result = pg_embedded_exec("SELECT nspname FROM pg_namespace ORDER BY nspname");
	printf("Available schemas: ");
	print_result(result);
	pg_embedded_free_result(result);
	
	result = pg_embedded_exec("SHOW search_path");
	printf("Current search_path: ");
	print_result(result);
	pg_embedded_free_result(result);
	
	result = pg_embedded_exec("SELECT relname, relnamespace::regnamespace FROM pg_class WHERE relkind = 'r' AND relnamespace NOT IN (SELECT oid FROM pg_namespace WHERE nspname LIKE 'pg_%' OR nspname = 'information_schema')");
	printf("User tables: ");
	print_result(result);
	pg_embedded_free_result(result);
	
	printf("\nShutting down again...\n");
	pg_embedded_shutdown();
	
	return 0;
}
