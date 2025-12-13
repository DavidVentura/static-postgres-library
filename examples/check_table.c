#include <stdio.h>
#include "pgembedded.h"

static void print_result(pg_result *result)
{
	uint64_t row;
	int col;
	
	if (!result) {
		printf("NULL result\n");
		return;
	}
	printf("Status: %d, Rows: %llu, Cols: %d\n", result->status, (unsigned long long) result->rows, result->cols);
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
}

int main(int argc, char **argv)
{
	pg_result *result;
	
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <data_directory>\n", argv[0]);
		return 1;
	}
	
	if (pg_embedded_init(argv[1], "postgres", "postgres") != 0) {
		fprintf(stderr, "ERROR: Init failed: %s\n", pg_embedded_error_message());
		return 1;
	}
	
	printf("Check if test_embedded table exists:\n");
	result = pg_embedded_exec("SELECT COUNT(*) FROM pg_class WHERE relname = 'test_embedded' AND relkind = 'r'");
	print_result(result);
	pg_embedded_free_result(result);
	
	printf("\nCreate table if it doesn't exist:\n");
	result = pg_embedded_exec("CREATE TABLE IF NOT EXISTS test_embedded (id SERIAL PRIMARY KEY, name TEXT, value INTEGER)");
	if (result && result->status < 0) {
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	} else {
		printf("Table created/exists\n");
	}
	pg_embedded_free_result(result);
	
	printf("\nInsert some test data:\n");
	result = pg_embedded_exec("INSERT INTO test_embedded (name, value) VALUES ('Alice', 100), ('Bob', 200) RETURNING id, name, value");
	print_result(result);
	pg_embedded_free_result(result);
	
	printf("\nQuery the data:\n");
	result = pg_embedded_exec("SELECT * FROM test_embedded ORDER BY id");
	print_result(result);
	pg_embedded_free_result(result);
	
	printf("\nShutdown...\n");
	pg_embedded_shutdown();
	
	printf("\n=== REOPEN ===\n");
	if (pg_embedded_init(argv[1], "postgres", "postgres") != 0) {
		fprintf(stderr, "ERROR: Re-init failed: %s\n", pg_embedded_error_message());
		return 1;
	}
	
	printf("\nAfter reopen - check if table exists:\n");
	result = pg_embedded_exec("SELECT COUNT(*) FROM pg_class WHERE relname = 'test_embedded' AND relkind = 'r'");
	print_result(result);
	pg_embedded_free_result(result);
	
	printf("\nAfter reopen - query the data:\n");
	result = pg_embedded_exec("SELECT * FROM test_embedded ORDER BY id");
	if (result && result->status < 0) {
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	print_result(result);
	pg_embedded_free_result(result);
	
	printf("\nShutdown again...\n");
	pg_embedded_shutdown();
	
	return 0;
}
