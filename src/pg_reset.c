#include "postgres.h"
#include "pgembedded.h"

#include <stdint.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

#include "libpq/pqsignal.h"
#include "miscadmin.h"
#include "storage/proc.h"
#include "tcop/tcopprot.h"
#include "utils/guc.h"
#include "utils/memutils.h"

extern void ResetGUCState(void);
extern void ResetMdState(void);
extern void ResetPortalState(void);
extern void ResetTransactionState(void);
extern void ResetUserIdState(void);
extern void ResetXLogState(void);
extern void ResetNamespaceState(void);
extern void ResetDynaHashState(void);
extern void ResetFileDescriptorState(void);
extern void ResetIPCState(void);
extern void ResetCatalogCacheState(void);
extern void ResetSmgrState(void);
extern void ResetRelCacheState(void);

void reset_state(void)
{
	/*
	 * Reset ALL global state to match pre-init conditions.
	 * shmem_exit() freed the memory but left dangling pointers.
	 */

	/* 1. Reset directly accessible memory context pointers */
	TopMemoryContext = NULL;
	ErrorContext = NULL;
	MessageContext = NULL;
	CurrentMemoryContext = NULL;
	CacheMemoryContext = NULL;
	TopTransactionContext = NULL;

	/* 2. Reset resource owners */
	CurrentResourceOwner = NULL;
	AuxProcessResourceOwner = NULL;

	/* 3. Reset processing mode and backend type */
	Mode = InitProcessing;
	MyBackendType = B_INVALID;

	/* 4. Reset database and config */
	MyDatabaseId = InvalidOid;
	MyDatabaseTableSpace = InvalidOid;
	MyProcPid = 0;
	MyProc = NULL;
	DataDir = NULL;
	ConfigFileName = NULL;

	/* 5. Reset signal masks to initial state */
	sigemptyset(&BlockSig);
	sigemptyset(&StartupBlockSig);

	/* 6. Reset output destination */
	whereToSendOutput = DestDebug;

	/* 7. Reset timestamps */
	MyStartTime = 0;
	MyStartTimestamp = 0;
	PgStartTime = 0;

	/* 8. Reset shared memory and cache state */
	extern unsigned long UsedShmemSegID;
	extern bool criticalRelcachesBuilt;
	extern bool criticalSharedRelcachesBuilt;
	UsedShmemSegID = 0;
	criticalRelcachesBuilt = false;
	criticalSharedRelcachesBuilt = false;

	/* 8. Reset notification queue */
	reset_notification_queue();

	/* 10. Reset static state in other PostgreSQL subsystems */
	ResetGUCState();
	ResetMdState();
	ResetSmgrState();
	ResetPortalState();
	ResetTransactionState();
	ResetUserIdState();
	ResetXLogState();
	ResetNamespaceState();
	ResetDynaHashState();
	ResetFileDescriptorState();
	ResetIPCState();
	ResetCatalogCacheState();
	ResetRelCacheState();
}

