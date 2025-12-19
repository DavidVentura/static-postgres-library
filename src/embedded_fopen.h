/*
 * embedded_fopen.h - File opening wrapper that supports embedded files
 */
#ifndef EMBEDDED_FOPEN_H
#define EMBEDDED_FOPEN_H

#include <stdio.h>
#include <stdbool.h>

extern FILE *embedded_AllocateFile(const char *path, const char *mode);
extern bool has_embedded_file(const char *path);
extern char *get_embedded_file_data(const char *path, int *length);

#endif
