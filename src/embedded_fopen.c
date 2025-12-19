/*
 * embedded_fopen.c - File opening wrapper that supports embedded files
 */
#include "postgres.h"
#include "extensions.h"
#include "embedded_timezone.h"
#include "storage/fd.h"
#include <string.h>

extern StaticExtensionLib *get_registered_libraries(void);

static bool
path_ends_with(const char *full_path, const char *suffix)
{
	size_t full_len = strlen(full_path);
	size_t suffix_len = strlen(suffix);

	if (suffix_len > full_len)
		return false;

	return strcmp(full_path + (full_len - suffix_len), suffix) == 0;
}

static const EmbeddedFile *
lookup_embedded_file(const char *path)
{
	StaticExtensionLib *lib;
	const EmbeddedFile *tz_file;

	for (lib = get_registered_libraries(); lib != NULL; lib = lib->next)
	{
		if (lib->control_file && path_ends_with(path, lib->control_file->filename))
			return lib->control_file;

		if (lib->script_file && path_ends_with(path, lib->script_file->filename))
			return lib->script_file;
	}

	tz_file = get_embedded_timezone_file();
	if (tz_file && path_ends_with(path, tz_file->filename))
		return tz_file;

	return NULL;
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

bool
has_embedded_file(const char *path)
{
	bool ret = lookup_embedded_file(path) != NULL;
	return ret;
}

char *
get_embedded_file_data(const char *path, int *length)
{
	const EmbeddedFile *file;

	file = lookup_embedded_file(path);
	if (!file)
		return NULL;

	if (length)
		*length = file->len;

	return (char *) file->data;
}
