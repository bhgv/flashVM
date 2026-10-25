/* FlashVM: Public embedding API declarations: sqlite.
 * File: include/flashvm/sqlite.h.
 */

#ifndef FLASHVM_SQLITE_H
/* Define FLASHVM SQLITE H constant or compilation switch. */
#define FLASHVM_SQLITE_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct FVM_SQLite FVM_SQLite;
/* Optional SQLite adapter; read/write requires host-granted database path. */


/* Open sqlite. */
FVM_API FVM_Result fvm_sqlite_open(const char *path, FVM_SQLite **out);


/* Implement sqlite exec for this subsystem. */
FVM_API FVM_Result fvm_sqlite_exec(FVM_SQLite *, const char *sql);


/* Implement sqlite query int for this subsystem. */
FVM_API FVM_Result fvm_sqlite_query_int(FVM_SQLite *, const char *sql, long *out);


/* Close sqlite. */
FVM_API void fvm_sqlite_close(FVM_SQLite *);
#ifdef __cplusplus
}
#endif
#endif
