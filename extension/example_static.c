/*
 * example_static - Static extension example for PostgreSQL
 *
 * This demonstrates a statically-linked extension that registers itself
 * at startup without requiring dynamic library loading.
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "extensions.h"

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

const StaticExtensionFunc example_static_functions[] = {
	{"add_one", add_one},
	{"hello_world", hello_world},
	{NULL, NULL}
};

const StaticExtensionFInfo example_static_finfo[] = {
	{"pg_finfo_add_one", pg_finfo_add_one},
	{"pg_finfo_hello_world", pg_finfo_hello_world},
	{NULL, NULL}
};

extern void example_static_PG_init(void);

void
register_example_static(void)
{
	register_static_extension(
		"example_static",
		example_static_PG_init,
		example_static_functions,
		example_static_finfo,
		NULL,
		NULL
	);
}
