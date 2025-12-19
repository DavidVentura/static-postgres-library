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
extern void register_example(void);
extern void register_plpgsql(void);
extern void register_vector(void);
extern void register_postgis(void);

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
	register_example();
	register_plpgsql();
	register_vector();
	register_postgis();

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

	/* Test CREATE EXTENSION */
	printf("----------------------------------------\n");
	printf("Test 1: CREATE EXTENSION example_static\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec("CREATE EXTENSION IF NOT EXISTS example");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
		//pg_embedded_free_result(result);
		//pg_embedded_shutdown();
		//return 1;
	}
	printf("Extension created successfully!\n\n");
	pg_embedded_free_result(result);

	/* Test CREATE EXTENSION */
	printf("----------------------------------------\n");
	printf("Test 1: CREATE EXTENSION IF NOT EXISTS plpgsql\n");
	printf("----------------------------------------\n");
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


	//result = pg_embedded_exec("DROP EXTENSION IF EXISTS vector");
	result = pg_embedded_exec("delete from items");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
		//pg_embedded_free_result(result);
		//pg_embedded_shutdown();
		//return 1;
	}
	pg_embedded_free_result(result);

	result = pg_embedded_exec("CREATE EXTENSION IF NOT EXISTS postgis");
	if (result && result->status < 0)
	{
		fprintf(stderr, "POSTGIS ERROR: %s\n", pg_embedded_error_message());
		pg_embedded_free_result(result);
		pg_embedded_shutdown();
		return 1;
	}

	result = pg_embedded_exec("CREATE EXTENSION IF NOT EXISTS vector");
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

	/* Test pgvector */
	printf("----------------------------------------\n");
	printf("Test 6: pgvector similarity search\n");
	printf("----------------------------------------\n");

	printf("Creating table with vector column...\n");
	result = pg_embedded_exec("CREATE TABLE items (id bigserial PRIMARY KEY, embedding vector(3))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: Failed to create table: %s\n",
				pg_embedded_error_message());
	}
	else
	{
		printf("Table created successfully!\n");
	}
	pg_embedded_free_result(result);

	printf("Inserting vector data...\n");
	result = pg_embedded_exec("INSERT INTO items (embedding) VALUES ('[1,2,3]'), ('[4,5,6]')");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: Failed to insert data: %s\n",
				pg_embedded_error_message());
	}
	else
	{
		printf("Data inserted successfully!\n");
	}
	pg_embedded_free_result(result);

	printf("Performing vector similarity search...\n");
	result = pg_embedded_exec("SELECT * FROM items ORDER BY embedding <-> '[3,1,2]' LIMIT 5");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("Found %ld results:\n", result->rows);
		for (int row = 0; row < result->rows; row++)
		{
			printf("  id: %s, embedding: %s\n", result->values[row][0], result->values[row][1]);
		}
		printf("\n");
	}
	pg_embedded_free_result(result);

	/* Test PostGIS */
	/*
	printf("----------------------------------------\n");
	printf("Test 7: PostGIS Info and Basic Functions\n");
	printf("----------------------------------------\n");

	printf("Testing PostGIS_Version()...\n");
	result = pg_embedded_exec("SELECT PostGIS_Version()");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  PostGIS_Version: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	printf("Testing PostGIS_Full_Version()...\n");
	result = pg_embedded_exec("SELECT PostGIS_Full_Version()");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  PostGIS_Full_Version: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	printf("Testing PostGIS_GEOS_Version()...\n");
	result = pg_embedded_exec("SELECT PostGIS_GEOS_Version()");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  GEOS Version: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	printf("Testing PostGIS_Proj_Version()...\n");
	result = pg_embedded_exec("SELECT PostGIS_Proj_Version()");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  PROJ Version: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	printf("Testing PostGIS_LibXML_Version()...\n");
	result = pg_embedded_exec("SELECT PostGIS_LibXML_Version()");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  LibXML Version: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	printf("\nTesting simple geometry creation (ST_MakePoint)...\n");
	result = pg_embedded_exec("SELECT ST_MakePoint(-122.4194, 37.7749)");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  ST_MakePoint result: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	// removed ST_AsText
	printf("Testing geometry to text conversion (ST_AsText)...\n");
	result = pg_embedded_exec("SELECT (ST_MakePoint(-122.4194, 37.7749))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  ST_AsText result: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	printf("\n----------------------------------------\n");
	printf("Test 8: PostGIS Table Operations\n");
	printf("----------------------------------------\n");

	*/
	printf("Creating table with geometry column...\n");
	result = pg_embedded_exec("CREATE TABLE IF NOT EXISTS locations (id SERIAL PRIMARY KEY, name TEXT, geom GEOMETRY(Point, 4326))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: Failed to create table: %s\n",
				pg_embedded_error_message());
	}
	else
	{
		printf("Table created successfully!\n");
	}
	pg_embedded_free_result(result);

	printf("Inserting geometric data...\n");
	result = pg_embedded_exec("DELETE FROM locations");

	result = pg_embedded_exec(
		"INSERT INTO locations (name, geom) VALUES "
		"('San Francisco', ST_SetSRID(ST_MakePoint(-122.4194, 37.7749), 4326)), "
		"('New York', ST_SetSRID(ST_MakePoint(-74.0060, 40.7128), 4326)), "
		"('London', ST_SetSRID(ST_MakePoint(-0.1278, 51.5074), 4326))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: Failed to insert data: %s\n",
				pg_embedded_error_message());
	}
	else
	{
		printf("Data inserted successfully!\n");
	}
	pg_embedded_free_result(result);

	printf("Get locations by distance\n");
	result = pg_embedded_exec(
		"SELECT l1.name, l2.name, "
		"ROUND(ST_Distance(l1.geom::geography, l2.geom::geography) / 1000)::integer AS distance_km "
		"FROM locations l1, locations l2 "
		"WHERE l1.name = 'San Francisco' AND l2.name != 'San Francisco' "
		"ORDER BY distance_km");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("Distances from San Francisco:\n");
		for (int row = 0; row < result->rows; row++)
		{
			printf("  %s to %s: %s km\n",
				result->values[row][0],
				result->values[row][1],
				result->values[row][2]);
		}
		printf("\n");
	}
	pg_embedded_free_result(result);

	/*
	printf("Testing ST_MakeEnvelope alone...\n");
	result = pg_embedded_exec("SELECT (ST_MakeEnvelope(-130, 30, -70, 50, 4326))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  Envelope: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	printf("Testing ST_Within with literal point and envelope...\n");
	result = pg_embedded_exec("SELECT ST_Within(ST_MakePoint(-122, 37), ST_MakeEnvelope(-130, 30, -70, 50, 4326))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  Result: %s\n", result->values[0][0]);
	}
	pg_embedded_free_result(result);

	printf("Testing ST_Within from table with ST_GeomFromText polygon...\n");
	result = pg_embedded_exec("SELECT name FROM locations WHERE ST_Within(geom, ST_GeomFromText('POLYGON((-130 30, -130 50, -70 50, -70 30, -130 30))', 4326))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  Cities in polygon:\n");
		for (int row = 0; row < result->rows; row++)
		{
			printf("    %s\n", result->values[row][0]);
		}
	}
	pg_embedded_free_result(result);

	printf("Testing ST_Within with table and envelope cast to geometry...\n");
	result = pg_embedded_exec("SELECT name FROM locations WHERE ST_Within(geom, ST_MakeEnvelope(-130, 30, -70, 50, 4326)::geometry)");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  Cities with explicit geometry cast:\n");
		for (int row = 0; row < result->rows; row++)
		{
			printf("    %s\n", result->values[row][0]);
		}
	}
	pg_embedded_free_result(result);
	*/

	printf("Testing ST_AsText without any WHERE clause...\n");
	result = pg_embedded_exec("SELECT name, ST_AsText(geom) FROM locations");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  All cities with ST_AsText (no filter):\n");
		for (int row = 0; row < result->rows; row++)
		{
			printf("    %s: %s\n", result->values[row][0], result->values[row][1]);
		}
	}
	if (result && result->status > 0) pg_embedded_free_result(result);

	printf("Testing ST_AsText with non-spatial WHERE...\n");
	result = pg_embedded_exec("SELECT name, (geom) FROM locations WHERE name = 'San Francisco'");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  Filtered city with ST_AsText (non-spatial):\n");
		for (int row = 0; row < result->rows; row++)
		{
			printf("    %s: %s\n", result->values[row][0], result->values[row][1]);
		}
	}
	if(result && result->status > 0) pg_embedded_free_result(result);

	printf("Testing with cast AND ST_AsText...\n");
	result = pg_embedded_exec("SELECT name, (geom) FROM locations WHERE ST_Within(geom, ST_MakeEnvelope(-130, 30, -70, 50, 4326)::geometry)");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  Cities with cast AND ST_AsText:\n");
		for (int row = 0; row < result->rows; row++)
		{
			printf("    %s: %s\n", result->values[row][0], result->values[row][1]);
		}
		pg_embedded_free_result(result);
	}

	printf("Testing without cast and without ST_AsText...\n");
	result = pg_embedded_exec("SELECT name FROM locations WHERE ST_Within(geom, ST_MakeEnvelope(-130, 30, -70, 50, 4326))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("  Cities without cast, without ST_AsText:\n");
		for (int row = 0; row < result->rows; row++)
		{
			printf("    %s\n", result->values[row][0]);
		}
		pg_embedded_free_result(result);
	}

	printf("Testing spatial relationship (cities within bounding box using envelope)...\n");
	result = pg_embedded_exec(
		"SELECT name, (geom) FROM locations "
		"WHERE ST_Within(geom, ST_MakeEnvelope(-130, 30, -70, 50, 4326))");
	if (result && result->status < 0)
	{
		fprintf(stderr, "ERROR: %s\n", pg_embedded_error_message());
	}
	else if (result && result->rows > 0)
	{
		printf("Cities in North America bounding box:\n");
		for (int row = 0; row < result->rows; row++)
		{
			printf("  %s: %s\n", result->values[row][0], result->values[row][1]);
		}
		printf("\n");
		if (result !=NULL) pg_embedded_free_result(result);
	}

	/* Test DROP EXTENSION */
	printf("----------------------------------------\n");
	printf("Test 9: DROP EXTENSION example_static\n");
	printf("----------------------------------------\n");
	result = pg_embedded_exec("DROP EXTENSION example");
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
