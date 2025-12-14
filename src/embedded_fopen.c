/*
 * embedded_fopen.c - File opening wrapper that supports embedded files
 */
#include "postgres.h"
#include "extensions.h"
#include "embedded_timezone.h"
#include "storage/fd.h"
#include <string.h>

extern StaticExtensionLib *get_registered_libraries(void);

static const EmbeddedFile *
lookup_embedded_file(const char *path)
{
	StaticExtensionLib *lib;
	const EmbeddedFile *tz_file;

	for (lib = get_registered_libraries(); lib != NULL; lib = lib->next)
	{
		if (lib->control_file && strcmp(lib->control_file->filename, path) == 0)
			return lib->control_file;

		if (lib->script_file && strcmp(lib->script_file->filename, path) == 0)
			return lib->script_file;
	}

	tz_file = get_embedded_timezone_file();
	if (tz_file && strcmp(tz_file->filename, path) == 0)
		return tz_file;

	return NULL;
}

FILE *
embedded_fopen(const char *path, const char *mode)
{
	const EmbeddedFile *file;

	file = lookup_embedded_file(path);
	if (file)
		return fmemopen((void *) file->data, file->len, mode);

	return fopen(path, mode);
}

FILE *
embedded_AllocateFile(const char *path, const char *mode)
{
	const EmbeddedFile *file;

	file = lookup_embedded_file(path);
	if (file)
		return fmemopen((void *) file->data, file->len, mode);

	return AllocateFile(path, mode);
}
