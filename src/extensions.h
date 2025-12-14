/*
 * extensions.h - Static extension support for embedded PostgreSQL
 */
#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include "postgres.h"
#include "fmgr.h"

typedef void (*PG_init_t) (void);
typedef const Pg_finfo_record *(*PGFInfoFunction) (void);

typedef struct StaticExtensionFunc
{
	const char *funcname;
	PGFunction	funcptr;
} StaticExtensionFunc;

typedef struct StaticExtensionFInfo
{
	const char *funcname;
	PGFInfoFunction finfofunc;
} StaticExtensionFInfo;

typedef struct StaticExtensionLib
{
	struct StaticExtensionLib *next;
	const char *library;
	PG_init_t	init_func;
	bool		init_called;
	const StaticExtensionFunc *functions;
	const StaticExtensionFInfo *finfo_functions;
} StaticExtensionLib;

#define STATIC_LIB_HANDLE_MAGIC 0xDEADBEEF

typedef struct StaticLibHandle
{
	uint32		magic;
	StaticExtensionLib *lib;
} StaticLibHandle;

extern void register_static_extension(const char *library,
									  PG_init_t init_func,
									  const StaticExtensionFunc *functions,
									  const StaticExtensionFInfo *finfo_functions);

extern void *pg_load_external_function(const char *filename,
									   const char *funcname,
									   bool signalNotFound,
									   void **filehandle);

extern void *pg_lookup_external_function(void *filehandle,
										 const char *funcname);

#endif
