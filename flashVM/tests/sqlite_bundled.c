#include <stdio.h>
#include <stdlib.h>
#include "flashvm/sqlite.h"
#include "sqlite3.h"
#include <string.h>


int main(void)
{
    FVM_SQLite *db = 0;
    sqlite3 *native_db = 0;
    sqlite3_stmt *stmt = 0;
    long value = 0;
    /* Verify the linked implementation, not merely an installed header. */
    if (!(strcmp(sqlite3_libversion(), "3.50.4") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_threadsafe() == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_compileoption_used("OMIT_LOAD_EXTENSION")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_open(":memory:", &native_db) == SQLITE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_prepare_v2(native_db, "SELECT ?1 + 1, ?2", -1, &stmt, 0) == SQLITE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_bind_int64(stmt, 1, (sqlite3_int64)2147483647 + 10) == SQLITE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_bind_text(stmt, 2, "FlashVM", -1, SQLITE_STATIC) == SQLITE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_step(stmt) == SQLITE_ROW))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_column_int64(stmt, 0) == (sqlite3_int64)2147483647 + 11))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(strcmp((const char *)sqlite3_column_text(stmt, 1), "FlashVM") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sqlite3_finalize(stmt) == SQLITE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Preserve the full SQLite integer ABI at both signed 64-bit boundaries. */
    {
        sqlite3_int64 maximum = ((sqlite3_int64)0x7fffffffU << 32) | (sqlite3_int64)0xffffffffU;
        sqlite3_int64 minimum = -maximum - 1;
        if (!(sizeof(sqlite3_int64) == 8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_prepare_v2(native_db,
                                 "SELECT ?1, ?2, CAST('9223372036854775807' AS INTEGER), "
                                 "CAST('-9223372036854775808' AS INTEGER)",
                                 -1,
                                 &stmt,
                                 0) == SQLITE_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_bind_int64(stmt, 1, maximum) == SQLITE_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_bind_int64(stmt, 2, minimum) == SQLITE_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_step(stmt) == SQLITE_ROW))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_column_int64(stmt, 0) == maximum))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_column_int64(stmt, 1) == minimum))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_column_int64(stmt, 2) == maximum))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_column_int64(stmt, 3) == minimum))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sqlite3_finalize(stmt) == SQLITE_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(sqlite3_close(native_db) == SQLITE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    if (!(fvm_sqlite_open(":memory:", &db) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_sqlite_exec(db, "CREATE TABLE t(v INTEGER UNIQUE); INSERT INTO t VALUES(42);") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_sqlite_exec(db, "BEGIN; INSERT INTO t VALUES(43); ROLLBACK;") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_sqlite_query_int(db, "SELECT count(*) FROM t", &value) == FVM_OK && value == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_sqlite_exec(db, "INSERT INTO t VALUES(42)") == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_sqlite_query_int(db, "SELECT sum(v) FROM t", &value) == FVM_OK && value == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_sqlite_exec(db, "invalid SQL") == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_sqlite_query_int(db, "SELECT v FROM t WHERE 0", &value) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_sqlite_close(db);
    db = (FVM_SQLite *)1;
    if (!(fvm_sqlite_open(0, &db) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_sqlite_close(0);
    return 0;
}
