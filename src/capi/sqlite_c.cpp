#include "flashvm/sqlite.h"
#include <sqlite3.h>
#include <new>
struct FVM_SQLite
{
    sqlite3 *db;
};
extern "C"
{
    FVM_Result fvm_sqlite_open(const char *path, FVM_SQLite **out)
    {
        FVM_SQLite *handle;
        if (!path || !out)
            return FVM_INVALID_ARGUMENT;
        *out = 0;
        handle = new (std::nothrow) FVM_SQLite;
        if (!handle)
            return FVM_OUT_OF_MEMORY;
        handle->db = 0;
        if (sqlite3_open_v2(path, &handle->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0) !=
            SQLITE_OK)
        {
            if (handle->db)
                sqlite3_close(handle->db);
            delete handle;
            return FVM_ERROR;
        }
        *out = handle;
        return FVM_OK;
    }
    FVM_Result fvm_sqlite_exec(FVM_SQLite *h, const char *sql)
    {
        if (!h || !sql)
            return FVM_INVALID_ARGUMENT;
        return sqlite3_exec(h->db, sql, 0, 0, 0) == SQLITE_OK ? FVM_OK : FVM_ERROR;
    }
    FVM_Result fvm_sqlite_query_int(FVM_SQLite *h, const char *sql, long *out)
    {
        sqlite3_stmt *stmt = 0;
        int rc;
        if (!h || !sql || !out)
            return FVM_INVALID_ARGUMENT;
        if (sqlite3_prepare_v2(h->db, sql, -1, &stmt, 0) != SQLITE_OK)
            return FVM_ERROR;
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
            *out = (long)sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        return rc == SQLITE_ROW ? FVM_OK : FVM_ERROR;
    }
    void fvm_sqlite_close(FVM_SQLite *h)
    {
        if (h)
        {
            sqlite3_close(h->db);
            delete h;
        }
    }
}
