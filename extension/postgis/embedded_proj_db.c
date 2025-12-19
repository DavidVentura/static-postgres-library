#include <sqlite3.h>
#include "proj_db.h"
#include <stddef.h>

sqlite3* get_embedded_proj_db() {
    sqlite3 *db = NULL;
    int rc;
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        return NULL;
    }

    rc = sqlite3_deserialize(db, "main", proj_db, proj_db_len, proj_db_len,
                             SQLITE_DESERIALIZE_READONLY);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return NULL;
    }

    return db;
}
