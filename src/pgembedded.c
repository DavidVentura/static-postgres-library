/*-------------------------------------------------------------------------
 *
 * pgembedded.c
 *	  PostgreSQL Embedded API implementation
 *
 * This file implements a simple embedded database interface that wraps
 * PostgreSQL's single-user mode and SPI (Server Programming Interface).
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 *
 * src/backend/embedded/pgembedded.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "pgembedded.h"
#include "initdb_embedded.h"

#include "access/xact.h"
#include "access/xlog.h"
#include "executor/spi.h"
#include "libpq/libpq.h"
#include "libpq/pqsignal.h"
#include "postmaster/postmaster.h"
#include "storage/ipc.h"
#include "storage/proc.h"
#include "tcop/tcopprot.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/portal.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"
#include "commands/async.h"

// from fd.c
#define  NUM_RESERVED_FDS 10

/* Notification queue for embedded mode */

/* Error message buffer shared across the API */
char pg_error_msg[1024] = {0};

void InitStandaloneProcess_(const char *argv0);

typedef void (*cleanup_fn)(void);
void library_cleanup(void);

static cleanup_fn handlers[32];
static int handler_count = 0;

extern void reset_state();

int __wrap_atexit(cleanup_fn func) {
    printf("Registering handler %d\n", handler_count);
    if (handler_count >= 32) return -1;
    handlers[handler_count++] = func;
    return 0;
}

void execute_atexit(void) {
    for (int i = handler_count - 1; i >= 0; i--) {
	printf("Executing atexit handler %d\n", i);
        handlers[i]();
    }
    handler_count = 0;
}


/* Static state */
static bool pg_initialized = false;
static char original_cwd[MAXPGPATH] = {0};

/* Pre-initialization config settings */
static struct {
	bool fsync;
	bool synchronous_commit;
	bool full_page_writes;
} preinit_config = {
	.fsync = true,                  /* default: enabled */
	.synchronous_commit = true,     /* default: enabled */
	.full_page_writes = true        /* default: enabled */
};


/*
 * pg_embedded_initdb
 *
 * Initialize a new PostgreSQL data directory in-process.
 * This creates the system catalogs and template databases without
 * forking external processes.
 */
int
pg_embedded_initdb(const char *data_dir, const char *username,
				   const char *encoding, const char *locale)
{
	int ret;

	if (!data_dir || !username)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "data_dir and username are required");
		return -1;
	}

	/* Call the in-process initdb implementation */
	reset_state();
	ret = pg_embedded_initdb_main(data_dir, username, encoding, locale);

	if (ret != 0)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "initdb failed");
		return -1;
	}

	return 0;
}

/*
 * pg_embedded_init_internal
 *
 * Initialize PostgreSQL in single-user embedded mode
 * If allow_system_table_mods is true, enables modification of system catalogs
 */
static int
pg_embedded_init_internal(const char *data_dir, const char *dbname,
						  const char *username, bool allow_system_table_mods)
{
	if (pg_initialized)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Already initialized");
		return 0;				/* Already initialized, not an error */
	}

	if (!data_dir || !dbname || !username)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Invalid arguments");
		return -1;
	}

	PG_TRY();
	{
		/*
		 * Essential early initialization (from main.c)
		 * Must happen before anything else, including InitStandaloneProcess
		 */
		MyProcPid = getpid();
		MyStartTime = time(NULL);

		/* Initialize memory context system - CRITICAL! */
		MemoryContextInit();

		/* Save current working directory so we can restore it on shutdown */
		if (!getcwd(original_cwd, MAXPGPATH))
		{
			snprintf(pg_error_msg, sizeof(pg_error_msg),
					 "Failed to get current working directory");
			PG_RE_THROW();
		}

		/* Set data directory */
		SetDataDir(data_dir);

		/* Initialize as standalone backend */
		InitStandaloneProcess_(progname);


		/*
		 * Set up signal handlers for standalone mode.
		 * This is critical - without these, checkpoints and other operations
		 * that try to use signals will corrupt the stack.
		 */
		pqsignal(SIGHUP, SIG_IGN);  /* ignore config reload in embedded mode */
		pqsignal(SIGINT, SIG_IGN);   /* ignore interrupts in embedded mode */
		pqsignal(SIGTERM, SIG_IGN);  /* ignore term signals */
		pqsignal(SIGQUIT, SIG_IGN);  /* ignore quit */
		pqsignal(SIGPIPE, SIG_IGN);  /* ignore broken pipe */
		pqsignal(SIGUSR1, SIG_IGN);  /* ignore SIGUSR1 (used for checkpoints in multi-user mode) */
		pqsignal(SIGUSR2, SIG_IGN);  /* ignore SIGUSR2 */

		/* Initialize configuration */
		InitializeGUCOptions();

		/*
		 * Apply pre-initialization performance config
		 */
		SetConfigOption("fsync", preinit_config.fsync ? "true" : "false",
						PGC_POSTMASTER, PGC_S_ARGV);

		SetConfigOption("synchronous_commit", preinit_config.synchronous_commit ? "on" : "off",
						PGC_POSTMASTER, PGC_S_ARGV);

		SetConfigOption("full_page_writes", preinit_config.full_page_writes ? "on" : "off",
						PGC_POSTMASTER, PGC_S_ARGV);

		/*
		 * Enable system table modifications if requested (needed for initdb).
		 * This must be set before SelectConfigFiles() is called.
		 */
		if (allow_system_table_mods)
		{
			SetConfigOption("allow_system_table_mods", "true", PGC_POSTMASTER, PGC_S_ARGV);
		}
		else
		{
		}

		/* Load configuration files */
		SelectConfigFiles(data_dir, username);

		/* Validate and switch to data directory */
		checkDataDir();
		ChangeToDataDir();

		/* Create lockfile */
		CreateDataDirLockFile(false);

		/* Read control file */
		LocalProcessControlFile(false);

		/* Load shared libraries */
		process_shared_preload_libraries();

		/* Initialize MaxBackends */
		InitializeMaxBackends();

		/*
		 * We don't need postmaster child slots in single-user mode, but
		 * initialize them anyway to avoid having special handling.
		 */
		InitPostmasterChildSlots();

		/* Initialize size of fast-path lock cache. */
		InitializeFastPathLocks();

		/*
		 * Give preloaded libraries a chance to request additional shared memory.
		 */
		process_shmem_requests();

		/*
		 * Now that loadable modules have had their chance to request additional
		 * shared memory, determine the value of any runtime-computed GUCs that
		 * depend on the amount of shared memory required.
		 */
		InitializeShmemGUCs();

		/*
		 * Now that modules have been loaded, we can process any custom resource
		 * managers specified in the wal_consistency_checking GUC.
		 */
		InitializeWalConsistencyChecking();

		/*
		 * Create shared memory etc.  (Nothing's really "shared" in single-user
		 * mode, but we must have these data structures anyway.)
		 */
		CreateSharedMemoryAndSemaphores();

		/*
		 * Estimate number of openable files.  This must happen after setting up
		 * semaphores, because on some platforms semaphores count as open files.
		 */
		// set_max_safe_fds();
		max_safe_fds = 1024 - NUM_RESERVED_FDS;

		/*
		 * Remember stand-alone backend startup time,roughly at the same point
		 * during startup that postmaster does so.
		 */
		PgStartTime = GetCurrentTimestamp();

		/*
		 * Create a per-backend PGPROC struct in shared memory. We must do this
		 * before we can use LWLocks.
		 */
		InitProcess();

		/* Early backend initialization */
		BaseInit();

		/* Connect to specified database */
		InitPostgres(dbname, InvalidOid, username, InvalidOid, 0, NULL);


		/* Set processing mode to normal */
		SetProcessingMode(NormalProcessing);

		/* Disable output to stdout/stderr */
		whereToSendOutput = DestNone;

		/* Register notification hook to capture NOTIFY messages */
		install_notification_hook();

		/*
		 * Create the memory context for query processing.
		 * MessageContext is used for query execution and is reset after each query.
		 */
		MessageContext = AllocSetContextCreate(TopMemoryContext,
											   "MessageContext",
											   ALLOCSET_DEFAULT_SIZES);

		/*
		 * Perform an empty transaction to finalize SPI setup.
		 * This ensures the system is ready for query execution.
		 */
		/*
		StartTransactionCommand();
		if (SPI_connect() != SPI_OK_CONNECT)
		{
			snprintf(pg_error_msg, sizeof(pg_error_msg), "SPI_connect failed");
			AbortCurrentTransaction();
			return -1;
		}
		SPI_finish();
		CommitTransactionCommand();
		*/

		pg_initialized = true;
	}
	PG_CATCH();
	{
		ErrorData  *edata;

		/* Get error data */
		edata = CopyErrorData();
		FlushErrorState();

		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "Initialization failed: %s", edata->message);

		FreeErrorData(edata);
		return -1;
	}
	PG_END_TRY();

	return 0;
}

/*
 * pg_embedded_init
 *
 * Public wrapper for normal use (no system table mods)
 */
int
pg_embedded_init(const char *data_dir, const char *dbname, const char *username)
{
	return pg_embedded_init_internal(data_dir, dbname, username, false);
}

/*
 * pg_embedded_init_with_system_mods
 *
 * Initialize with system table modifications enabled (for initdb)
 */
int
pg_embedded_init_with_system_mods(const char *data_dir, const char *dbname, const char *username)
{
	return pg_embedded_init_internal(data_dir, dbname, username, true);
}

/*
 * copy_tuptable
 *
 * Copy SPI tuple table results into a pg_result structure.
 * Returns 0 on success, -1 on failure.
 */
static int
copy_tuptable(pg_result *result, SPITupleTable *tuptable)
{
	TupleDesc	tupdesc = tuptable->tupdesc;
	uint64_t	row;
	int		col;


	/* Allocate column names array */
	result->colnames = (char **) malloc(result->cols * sizeof(char *));
	if (!result->colnames)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Out of memory");
		return -1;
	}

	/* Copy column names */
	for (col = 0; col < result->cols; col++)
	{
		Form_pg_attribute attr = TupleDescAttr(tupdesc, col);

		result->colnames[col] = strdup(NameStr(attr->attname));
	}

	/* Allocate result matrix */
	result->values = (char ***) malloc(result->rows * sizeof(char **));
	if (!result->values)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Out of memory");
		return -1;
	}

	/* Copy data for each row */
	for (row = 0; row < result->rows; row++)
	{
		HeapTuple	tuple = tuptable->vals[row];

		result->values[row] = (char **) malloc(result->cols * sizeof(char *));
		if (!result->values[row])
		{
			snprintf(pg_error_msg, sizeof(pg_error_msg), "Out of memory");
			return -1;
		}

		/* Get each column value */
		for (col = 0; col < result->cols; col++)
		{
			char	   *str;

			str = SPI_getvalue(tuple, tupdesc, col + 1);

			if (str == NULL)
				result->values[row][col] = NULL;
			else
			{
				result->values[row][col] = strdup(str);
				pfree(str);
			}
		}
	}

	return 0;
}

/*
 * pg_embedded_exec
 *
 * Execute SQL query and return results
 */
pg_result *
pg_embedded_exec(const char *query)
{
	pg_result  *result;
	int			ret;
	volatile bool	implicit_tx = false;
	volatile bool	spi_connected = false;
	volatile bool	snapshot_pushed = false;
	ErrorData  *edata;


	if (!pg_initialized)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not initialized");
		return NULL;
	}

	if (!query)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "NULL query");
		return NULL;
	}

	/* Allocate result structure */
	result = (pg_result *) malloc(sizeof(pg_result));
	if (!result)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Out of memory");
		return NULL;
	}

	memset(result, 0, sizeof(pg_result));

	PG_TRY();
	{

		/*
		 * Transaction Handling Strategy:
		 * If we are NOT in a transaction, we act as "Auto-commit":
		 * Start -> Exec -> Commit.
		 * If we ARE in a transaction (via pg_embedded_begin), we just Exec.
		 */
		if (!IsTransactionState())
		{
			StartTransactionCommand();
			implicit_tx = true;
		}
		else
		{
		}

		/*
		 * SPI requires a snapshot to be active.
		 * Push an active snapshot for query execution.
		 */
		PushActiveSnapshot(GetTransactionSnapshot());
		snapshot_pushed = true;

		if (SPI_connect() != SPI_OK_CONNECT)
		{
			snprintf(pg_error_msg, sizeof(pg_error_msg), "SPI_connect failed");
			result->status = -1;
		}
		else
		{
			spi_connected = true;
			ret = SPI_execute(query, false, 0);	/* false = read-write, 0 = no
								 * row limit */

			result->status = ret;
			result->rows = SPI_processed;
			result->cols = 0;
			result->values = NULL;
			result->colnames = NULL;

			if (ret >= 0 && ret > 0 && SPI_tuptable != NULL)
			{
				result->cols = SPI_tuptable->tupdesc->natts;
				/*
				 * Copy data for queries with results (SELECT or RETURNING)
				 */
				if (copy_tuptable(result, SPI_tuptable) != 0)
				{
					pg_embedded_free_result(result);
					result = NULL;
				}
			}
			SPI_finish();
			spi_connected = false;
		}

		snapshot_pushed = false;
		PopActiveSnapshot();

		if (implicit_tx) {
			if (result != NULL && result->status >= 0) {
				CommitTransactionCommand();
			} else {
				AbortCurrentTransaction();
			}
		}
	}
	PG_CATCH();
	{
		fprintf(stderr, "[WARN] In PG_CATCH\n");

		edata = CopyErrorData();
		FlushErrorState();

		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "Query failed: %s", edata->message);

		if (snapshot_pushed) PopActiveSnapshot();
		if (spi_connected) SPI_finish();
		AbortCurrentTransaction();

		result->status = -1;
	}
	PG_END_TRY();

	return result;
}

/*
 * pg_embedded_free_result
 *
 * Free result structure
 */
void
pg_embedded_free_result(pg_result *result)
{
	uint64_t	row;
	int			col;

	if (!result)
		return;

	/* Free column names */
	if (result->colnames)
	{
		for (col = 0; col < result->cols; col++)
		{
			if (result->colnames[col])
				free(result->colnames[col]);
		}
		free(result->colnames);
	}

	/* Free data values */
	if (result->values)
	{
		for (row = 0; row < result->rows; row++)
		{
			if (result->values[row])
			{
				for (col = 0; col < result->cols; col++)
				{
					if (result->values[row][col])
						free(result->values[row][col]);
				}
				free(result->values[row]);
			}
		}
		free(result->values);
	}

	free(result);
}

/*
 * pg_embedded_begin
 *
 * Begin a transaction
 */
int
pg_embedded_begin(void)
{
	if (!pg_initialized)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not initialized");
		return -1;
	}

	if (IsTransactionState())
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Already in transaction");
		return -1;
	}

	PG_TRY();
	{
		StartTransactionCommand();
	}
	PG_CATCH();
	{
		ErrorData  *edata;

		edata = CopyErrorData();
		FlushErrorState();
		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "BEGIN failed: %s", edata->message);
		FreeErrorData(edata);
		AbortCurrentTransaction();
		return -1;
	}
	PG_END_TRY();

	return 0;
}

/*
 * pg_embedded_commit
 *
 * Commit current transaction using the C API
 */
int
pg_embedded_commit(void)
{
	if (!pg_initialized)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not initialized");
		return -1;
	}

	if (!IsTransactionState())
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not in transaction");
		return -1;
	}

	PG_TRY();
	{
		CommitTransactionCommand();
	}
	PG_CATCH();
	{
		ErrorData  *edata;

		edata = CopyErrorData();
		FlushErrorState();
		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "COMMIT failed: %s", edata->message);
		FreeErrorData(edata);
		AbortCurrentTransaction();
		return -1;
	}
	PG_END_TRY();

	return 0;
}

/*
 * pg_embedded_rollback
 *
 * Rollback current transaction using the C API
 */
int
pg_embedded_rollback(void)
{
	if (!pg_initialized)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not in transaction");
		return -1;
	}

	if (!IsTransactionState())
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not in transaction");
		return -1;
	}

	PG_TRY();
	{
		AbortCurrentTransaction();
	}
	PG_CATCH();
	{
		FlushErrorState();
	}
	PG_END_TRY();

	return 0;
}

/*
 * pg_embedded_set_config
 *
 * Set performance configuration before initialization
 *
 * IMPORTANT: Must be called BEFORE pg_embedded_init()
 */
void
pg_embedded_set_config(const pg_embedded_config *config)
{
	if (config)
	{
		preinit_config.fsync = config->fsync;
		preinit_config.synchronous_commit = config->synchronous_commit;
		preinit_config.full_page_writes = config->full_page_writes;
	}
}

/*
 * pg_embedded_error_message
 *
 * Get last error message
 */
const char *
pg_embedded_error_message(void)
{
	return pg_error_msg;
}

/*
 * pg_embedded_listen
 *
 * Register to listen for notifications on a channel
 * This is equivalent to SQL: LISTEN channel_name
 *
 * We call Async_Listen directly to bypass the SQL parser check that
 * blocks LISTEN in non-regular backends. In embedded mode, we handle
 * notification collection ourselves via pg_embedded_poll_notifications.
 */
int
pg_embedded_listen(const char *channel)
{
	if (!pg_initialized)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not initialized");
		return -1;
	}

	if (!channel || channel[0] == '\0')
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Channel name required");
		return -1;
	}

	PG_TRY();
	{
		bool implicit_tx = false;

		if (!IsTransactionState())
		{
			StartTransactionCommand();
			implicit_tx = true;
		}

		Async_Listen(channel);

		if (implicit_tx)
		{
			CommitTransactionCommand();
		}
	}
	PG_CATCH();
	{
		ErrorData *edata;

		edata = CopyErrorData();
		FlushErrorState();
		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "LISTEN failed: %s", edata->message);
		FreeErrorData(edata);
		AbortCurrentTransaction();
		return -1;
	}
	PG_END_TRY();

	return 0;
}

/*
 * pg_embedded_unlisten
 *
 * Stop listening for notifications on a channel
 * This is equivalent to SQL: UNLISTEN channel_name
 * If channel is NULL, unlisten from all channels (UNLISTEN *)
 */
int
pg_embedded_unlisten(const char *channel)
{
	if (!pg_initialized)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not initialized");
		return -1;
	}

	PG_TRY();
	{
		bool implicit_tx = false;

		if (!IsTransactionState())
		{
			StartTransactionCommand();
			implicit_tx = true;
		}

		if (channel == NULL)
		{
			Async_UnlistenAll();
		}
		else
		{
			Async_Unlisten(channel);
		}

		if (implicit_tx)
		{
			CommitTransactionCommand();
		}
	}
	PG_CATCH();
	{
		ErrorData *edata;

		edata = CopyErrorData();
		FlushErrorState();
		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "UNLISTEN failed: %s", edata->message);
		FreeErrorData(edata);
		AbortCurrentTransaction();
		return -1;
	}
	PG_END_TRY();

	return 0;
}

/*
 * pg_embedded_notify
 *
 * Send a notification on a channel with optional payload
 * This is equivalent to SQL: NOTIFY channel_name, 'payload'
 */
int
pg_embedded_notify(const char *channel, const char *payload)
{
	if (!pg_initialized)
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Not initialized");
		return -1;
	}

	if (!channel || channel[0] == '\0')
	{
		snprintf(pg_error_msg, sizeof(pg_error_msg), "Channel name required");
		return -1;
	}

	PG_TRY();
	{
		bool implicit_tx = false;

		if (!IsTransactionState())
		{
			StartTransactionCommand();
			implicit_tx = true;
		}

		Async_Notify(channel, payload ? payload : "");

		if (implicit_tx)
		{
			CommitTransactionCommand();
		}
	}
	PG_CATCH();
	{
		ErrorData *edata;

		edata = CopyErrorData();
		FlushErrorState();
		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "NOTIFY failed: %s", edata->message);
		FreeErrorData(edata);
		AbortCurrentTransaction();
		return -1;
	}
	PG_END_TRY();

	return 0;
}

/*
 * pg_embedded_shutdown
 *
 * Shutdown embedded PostgreSQL instance
 */
void
pg_embedded_shutdown(void)
{
	if (!pg_initialized)
		return;

	PG_TRY();
	{
		/*
		 * Use shmem_exit(0) instead of proc_exit(0).
		 * This runs all the internal PostgreSQL cleanup hooks
		 * (closing WAL, flushing buffers, releasing locks) but does NOT
		 * call exit() and kill the host application process.
		 */
		shmem_exit(0);
	}
	PG_CATCH();
	{
		/* Ignore errors during shutdown */
		FlushErrorState();
	}
	PG_END_TRY();

	execute_atexit();

	/*
	 * Restore original working directory so that relative paths work
	 * correctly if we re-initialize later.
	 */
	if (original_cwd[0] != '\0')
	{
		if (chdir(original_cwd) < 0)
		{
			fprintf(stderr, "Warning: Failed to restore working directory to %s\n", original_cwd);
		}
	}

	pg_initialized = false;
}

// Patched to not look for its own binary
void
InitStandaloneProcess_(const char *argv0)
{
	Assert(!IsPostmasterEnvironment);

	MyBackendType = B_STANDALONE_BACKEND;

	/*
	 * Start our win32 signal implementation
	 */
#ifdef WIN32
	pgwin32_signal_initialize();
#endif

	InitProcessGlobals();

	/* Initialize process-local latch support */
	InitializeWaitEventSupport();
	InitProcessLocalLatch();
	InitializeLatchWaitSet();

	/*
	 * For consistency with InitPostmasterChild, initialize signal mask here.
	 * But we don't unblock SIGQUIT or provide a default handler for it.
	 */
	pqinitmask();
	sigprocmask(SIG_SETMASK, &BlockSig, NULL);
}
