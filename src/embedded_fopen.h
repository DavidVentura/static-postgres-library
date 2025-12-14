/*
 * embedded_fopen.h - File opening wrapper that supports embedded files
 */
#ifndef EMBEDDED_FOPEN_H
#define EMBEDDED_FOPEN_H

#include <stdio.h>

extern FILE *embedded_fopen(const char *path, const char *mode);
extern FILE *embedded_AllocateFile(const char *path, const char *mode);

#endif
