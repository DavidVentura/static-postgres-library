/*
 * extensions.c - Static extension support implementation
 */
#include "postgres.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "utils/builtins.h"
#include "extensions.h"

static StaticExtensionLib *registered_libraries = NULL;

StaticExtensionLib *
get_registered_libraries(void)
{
	return registered_libraries;
}

void
register_static_extension(const char *library,
			  PG_init_t init_func,
			  const StaticExtensionFunc *functions,
			  const StaticExtensionFInfo *finfo_functions,
			  const EmbeddedFile *control_file,
			  const EmbeddedFile *script_file)
{
	StaticExtensionLib *lib;

	lib = (StaticExtensionLib *) malloc(sizeof(StaticExtensionLib));
	if (lib == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));

	lib->library = library;
	lib->init_func = init_func;
	lib->init_called = false;
	lib->functions = functions;
	lib->finfo_functions = finfo_functions;
	lib->control_file = control_file;
	lib->script_file = script_file;

	lib->next = registered_libraries;
	registered_libraries = lib;

	elog(DEBUG1, "Registered static extension: %s", library);
}

static char *
normalize_library_name(const char *name)
{
	char	   *result;
	const char *basename;
	char	   *dot;

	if (strncmp(name, "$libdir/", 8) == 0)
		name += 8;

	basename = last_dir_separator(name);
	if (basename)
		basename++;
	else
		basename = name;

	result = pstrdup(basename);

	dot = strrchr(result, '.');
	if (dot && (strcmp(dot, ".so") == 0 || strcmp(dot, ".dll") == 0 || strcmp(dot, ".dylib") == 0))
		*dot = '\0';

	return result;
}

static StaticExtensionLib *
lookup_static_library(const char *filename)
{
	StaticExtensionLib *lib;
	char	   *normalized_name;

	normalized_name = normalize_library_name(filename);

	for (lib = registered_libraries; lib != NULL; lib = lib->next)
	{
		if (strcmp(lib->library, normalized_name) == 0)
		{
			pfree(normalized_name);
			return lib;
		}
	}

	pfree(normalized_name);
	return NULL;
}

static const StaticExtensionFunc *
lookup_function_in_library(StaticExtensionLib *lib, const char *funcname)
{
	const StaticExtensionFunc *func;

	if (lib == NULL || lib->functions == NULL)
		return NULL;

	for (func = lib->functions; func->funcname != NULL; func++)
	{
		if (strcmp(func->funcname, funcname) == 0)
			return func;
	}

	return NULL;
}

static void
call_static_pg_init_once(StaticExtensionLib *lib)
{
	if (lib->init_func != NULL && !lib->init_called)
	{
		elog(DEBUG1, "Calling _PG_init for static library: %s", lib->library);
		(*lib->init_func) ();
		lib->init_called = true;
	}
}

void *
pg_load_external_function(const char *filename,
			  const char *funcname,
			  bool signalNotFound,
			  void **filehandle)
{
	StaticExtensionLib *lib;
	const StaticExtensionFunc *func;
	StaticLibHandle *handle;

	lib = lookup_static_library(filename);

	if (lib == NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FILE),
				 errmsg("could not find library \"%s\" in registered static extensions", filename),
				 errhint("The library must be registered via register_static_extension() before use.")));
	}

	call_static_pg_init_once(lib);

	func = lookup_function_in_library(lib, funcname);

	if (func == NULL)
	{
		if (signalNotFound)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_FUNCTION),
					 errmsg("could not find function \"%s\" in static library \"%s\"",
							funcname, lib->library)));
		return NULL;
	}

	if (filehandle)
	{
		handle = (StaticLibHandle *) malloc(sizeof(StaticLibHandle));
		if (handle == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("out of memory")));

		handle->magic = STATIC_LIB_HANDLE_MAGIC;
		handle->lib = lib;
		*filehandle = (void *) handle;
	}

	return (void *) func->funcptr;
}

void *
pg_lookup_external_function(void *filehandle, const char *funcname)
{
	StaticLibHandle *handle;
	const StaticExtensionFunc *func;
	const StaticExtensionFInfo *finfo;

	if (filehandle == NULL)
		return NULL;

	handle = (StaticLibHandle *) filehandle;

	if (handle->magic != STATIC_LIB_HANDLE_MAGIC)
	{
		elog(WARNING, "Invalid static library handle (bad magic number)");
		return NULL;
	}

	if (strncmp(funcname, "pg_finfo_", 9) == 0)
	{
		for (finfo = handle->lib->finfo_functions; finfo && finfo->funcname != NULL; finfo++)
		{
			if (strcmp(finfo->funcname, funcname) == 0)
				return (void *) finfo->finfofunc;
		}
		return NULL;
	}

	func = lookup_function_in_library(handle->lib, funcname);

	if (func == NULL)
		return NULL;

	return (void *) func->funcptr;
}
