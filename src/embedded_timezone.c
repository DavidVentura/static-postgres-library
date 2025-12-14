/*
 * embedded_timezone.c - Embedded timezone data
 */
#include "postgres.h"
#include "extensions.h"

#include "embedded_timezone_data.h"

static const EmbeddedFile timezone_file = {
	.filename = "Default",
	.data = timezone_default,
	.len = timezone_default_len
};

const EmbeddedFile *
get_embedded_timezone_file(void)
{
	return &timezone_file;
}
