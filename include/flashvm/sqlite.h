#ifndef FLASHVM_SQLITE_H
#define FLASHVM_SQLITE_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FVM_SQLite FVM_SQLite;
/* Optional SQLite adapter; read/write requires host-granted database path. */
FVM_API FVM_Result fvm_sqlite_open(const char *path, FVM_SQLite **out);
FVM_API FVM_Result fvm_sqlite_exec(FVM_SQLite *, const char *sql);
FVM_API FVM_Result fvm_sqlite_query_int(FVM_SQLite *, const char *sql, long *out);
FVM_API void fvm_sqlite_close(FVM_SQLite *);
#ifdef __cplusplus
}
#endif
#endif
