/* FlashVM: Public embedding API declarations: mutex.
 * File: include/flashvm/mutex.h.
 */

#ifndef FLASHVM_MUTEX_H
/* Define FLASHVM MUTEX H constant or compilation switch. */
#define FLASHVM_MUTEX_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FVM_Mutex FVM_Mutex;
    /* Context identities are process-local, monotonically assigned, never pointers. */
    /* Implement context identity for this subsystem. */
    FVM_API unsigned long fvm_context_identity(FVM_Context *);
    /* Create mutex. */
    FVM_API FVM_Result fvm_mutex_create(FVM_Context *, FVM_Mutex **);
    /* Retain mutex. */
    FVM_API FVM_Result fvm_mutex_retain(FVM_Mutex *);
    /* Release mutex. */
    FVM_API void fvm_mutex_release(FVM_Mutex *);
    /* Nonrecursive, owner-only release. Nonblocking contention returns NOT_FOUND.
       Blocking acquisition cooperatively observes the bound worker's stop request. */
    /* Update mutex. */
    FVM_API FVM_Result fvm_mutex_set(FVM_Context *, FVM_Mutex *, int blocking);
    /* Reset mutex. */
    FVM_API FVM_Result fvm_mutex_reset(FVM_Context *, FVM_Mutex *);
    /* Close mutex. */
    FVM_API FVM_Result fvm_mutex_close(FVM_Mutex *);
    /* Test whether mutex closed. */
    FVM_API FVM_Result fvm_mutex_is_closed(FVM_Mutex *, int *);
    /* Implement mutex touch for this subsystem. */
    FVM_API FVM_Result fvm_mutex_touch(FVM_Mutex *, int *);
    /* Implement mutex origin for this subsystem. */
    FVM_API unsigned long fvm_mutex_origin(FVM_Mutex *);
    /* Implement mutex identity for this subsystem. */
    FVM_API unsigned long fvm_mutex_identity(FVM_Mutex *);
    /* Implement extensions wrap mutex for this subsystem. */
    FVM_API FVM_Result fvm_extensions_wrap_mutex(FVM_Context *, FVM_Mutex *, FVM_Object **);
#ifdef __cplusplus
}
#endif
#endif
