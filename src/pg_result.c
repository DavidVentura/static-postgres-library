/*-------------------------------------------------------------------------
 *
 * pg_result.c
 *	  Result handling for PostgreSQL Embedded API
 *
 * This file implements zero-copy result accessors for the embedded API.
 * Data is kept as references to PostgreSQL's internal SPI structures
 * and only copied/allocated when necessary (e.g., toasted values).
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 *
 * src/backend/embedded/pg_result.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <string.h>

#include "pgembedded.h"
#include "executor/spi.h"
#include "utils/array.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"

/* Error message buffer (defined in pgembedded.c) */
extern char pg_error_msg[1024];

/*
 * pg_embedded_get_datum_raw
 *
 * Get raw Datum value as uint64_t (for by-value types)
 */
uint64_t
pg_embedded_get_datum_raw(pg_result *res, uint64_t row, int col, bool *isnull)
{
	HeapTuple	tuple;
	TupleDesc	tupdesc;
	Datum		datum;

	if (!res || row >= res->rows || col >= res->cols || !res->tuptable)
	{
		if (isnull)
			*isnull = true;
		return 0;
	}

	tuple = res->tuptable->vals[row];
	tupdesc = res->tuptable->tupdesc;

	datum = SPI_getbinval(tuple, tupdesc, col + 1, isnull);

	if (*isnull)
		return 0;

	return (uint64_t) datum;
}

/*
 * pg_embedded_get_bytes
 *
 * Get bytes from by-reference types with zero-copy when possible
 */
pg_bytes
pg_embedded_get_bytes(pg_result *res, uint64_t row, int col, bool *isnull)
{
	pg_bytes	result = {0};
	Datum		datum;
	TupleDesc	tupdesc;
	Form_pg_attribute attr;

	if (!res || row >= res->rows || col >= res->cols || !res->tuptable)
	{
		if (isnull)
			*isnull = true;
		return result;
	}

	tupdesc = res->tuptable->tupdesc;
	attr = TupleDescAttr(tupdesc, col);

	/* Get datum using SPI which handles tuple deforming */
	HeapTuple tuple = res->tuptable->vals[row];
	datum = heap_getattr(tuple, col + 1, tupdesc, isnull);

	if (*isnull)
		return result;

	/*
	 * Handle based on type length:
	 * - attlen == -1: varlena (variable length)
	 * - attlen > 0 && !attbyval: fixed-length pass-by-reference
	 * - attlen > 0 && attbyval: pass-by-value (not supported here)
	 */
	if (attr->attlen == -1)
	{
		/* Variable-length (varlena) type */
		struct varlena *v = PG_DETOAST_DATUM_PACKED(datum);

		result.data = VARDATA_ANY(v);
		result.len = VARSIZE_ANY_EXHDR(v);

		/* Check if detoasting allocated */
		if ((void*)v != DatumGetPointer(datum))
		{
			result.needs_free = 1;
		}
		else
		{
			result.needs_free = 0;
		}
	}
	else if (attr->attlen > 0 && !attr->attbyval)
	{
		/* Fixed-length pass-by-reference (e.g., name type - 64 bytes null-padded) */
		result.data = DatumGetPointer(datum);

		/* For name type (OID 19), find actual string length (it's null-padded) */
		if (attr->atttypid == 19)  /* NAMEOID */
		{
			result.len = strlen((const char*)result.data);
		}
		else
		{
			result.len = attr->attlen;
		}
		result.needs_free = 0;
	}
	else
	{
		/* Pass-by-value types not supported in get_bytes */
		*isnull = true;
	}

	return result;
}

/*
 * pg_embedded_free_bytes
 *
 * Free bytes if they were allocated (needs_free == true)
 */
void
pg_embedded_free_bytes(pg_bytes *bytes)
{
	if (bytes && bytes->needs_free && bytes->data)
	{
		/* Back up to the varlena header and free */
		void	   *ptr = (void *) ((char *) bytes->data - VARHDRSZ);

		pfree(ptr);
		bytes->data = NULL;
		bytes->len = 0;
		bytes->needs_free = 0;
	}
}

/*
 * pg_embedded_get_int32
 *
 * Get int32 value (zero-copy)
 */
int32_t
pg_embedded_get_int32(pg_result *res, uint64_t row, int col, bool *isnull)
{
	uint64_t	datum = pg_embedded_get_datum_raw(res, row, col, isnull);

	if (*isnull)
		return 0;

	return DatumGetInt32((Datum) datum);
}

/*
 * pg_embedded_get_int64
 *
 * Get int64 value (zero-copy)
 */
int64_t
pg_embedded_get_int64(pg_result *res, uint64_t row, int col, bool *isnull)
{
	uint64_t	datum = pg_embedded_get_datum_raw(res, row, col, isnull);

	if (*isnull)
		return 0;

	return DatumGetInt64((Datum) datum);
}

/*
 * pg_embedded_get_float64
 *
 * Get float64 value (zero-copy)
 */
double
pg_embedded_get_float64(pg_result *res, uint64_t row, int col, bool *isnull)
{
	uint64_t	datum = pg_embedded_get_datum_raw(res, row, col, isnull);

	if (*isnull)
		return 0.0;

	return DatumGetFloat8((Datum) datum);
}

/*
 * pg_embedded_get_bool
 *
 * Get boolean value (zero-copy)
 */
bool
pg_embedded_get_bool(pg_result *res, uint64_t row, int col, bool *isnull)
{
	uint64_t	datum = pg_embedded_get_datum_raw(res, row, col, isnull);

	if (*isnull)
		return false;

	return DatumGetBool((Datum) datum);
}

/*
 * pg_embedded_get_string_debug
 *
 * Get value as C string for debugging (always allocates)
 * Uses PostgreSQL's type output functions
 */
char *
pg_embedded_get_string_debug(pg_result *res, uint64_t row, int col)
{
	bool		isnull;
	Datum		datum;
	Oid			typoid;
	Oid			typoutput;
	bool		typIsVarlena;
	char	   *result;

	if (!res || row >= res->rows || col >= res->cols || !res->tuptable)
		return NULL;

	datum = pg_embedded_get_datum_raw(res, row, col, &isnull);

	if (isnull)
		return NULL;

	typoid = res->coltypes[col];

	/* Get the type's output function */
	getTypeOutputInfo(typoid, &typoutput, &typIsVarlena);

	/* Convert Datum to string using output function */
	result = OidOutputFunctionCall(typoutput, datum);

	/*
	 * Need to copy to malloc'd memory since caller expects to free with
	 * pg_embedded_free_string (uses free(), not pfree())
	 */
	if (result)
	{
		char	   *copy = strdup(result);

		pfree(result);
		return copy;
	}

	return NULL;
}

/*
 * pg_embedded_free_string
 *
 * Free string returned by pg_embedded_get_string_debug
 */
void
pg_embedded_free_string(char *str)
{
	if (str)
		free(str);
}

/*
 * pg_embedded_get_colnames
 *
 * Get column names (allocates)
 */
char **
pg_embedded_get_colnames(pg_result *res)
{
	char	  **colnames;
	TupleDesc	tupdesc;
	int			i;

	if (!res || !res->tuptable || res->cols == 0)
		return NULL;

	tupdesc = res->tuptable->tupdesc;

	colnames = (char **) malloc(res->cols * sizeof(char *));
	if (!colnames)
		return NULL;

	for (i = 0; i < res->cols; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(tupdesc, i);

		colnames[i] = strdup(NameStr(attr->attname));
		if (!colnames[i])
		{
			/* Cleanup on failure */
			while (--i >= 0)
				free(colnames[i]);
			free(colnames);
			return NULL;
		}
	}

	return colnames;
}

/*
 * pg_embedded_free_colnames
 *
 * Free column names returned by pg_embedded_get_colnames
 */
void
pg_embedded_free_colnames(char **colnames, int cols)
{
	int			i;

	if (!colnames)
		return;

	for (i = 0; i < cols; i++)
	{
		if (colnames[i])
			free(colnames[i]);
	}

	free(colnames);
}

/*
 * pg_embedded_free_result
 *
 * Free result structure (lightweight - just metadata)
 * SPI data stays alive until next query
 */
void
pg_embedded_free_result(pg_result *result)
{
	if (!result)
		return;

	/* Free column types array */
	if (result->coltypes)
		free(result->coltypes);

	/* Note: tuptable is NOT freed - it's owned by SPI until next query */

	free(result);
}
