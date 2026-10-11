#ifndef FLASHVM_MUTEX_H
#define FLASHVM_MUTEX_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FVM_Mutex FVM_Mutex;
    /* Context identities are process-local, monotonically assigned, never pointers. */
    FVM_API unsigned long fvm_context_identity(FVM_Context *);
    FVM_API FVM_Result fvm_mutex_create(FVM_Context *, FVM_Mutex **);
    FVM_API FVM_Result fvm_mutex_retain(FVM_Mutex *);
    FVM_API void fvm_mutex_release(FVM_Mutex *);
    /* Nonrecursive, owner-only release. Nonblocking contention returns NOT_FOUND.
       Blocking acquisition cooperatively observes the bound worker's stop request. */
    FVM_API FVM_Result fvm_mutex_set(FVM_Context *, FVM_Mutex *, int blocking);
    FVM_API FVM_Result fvm_mutex_reset(FVM_Context *, FVM_Mutex *);
    FVM_API FVM_Result fvm_mutex_close(FVM_Mutex *);
    FVM_API FVM_Result fvm_mutex_is_closed(FVM_Mutex *, int *);
    FVM_API FVM_Result fvm_mutex_touch(FVM_Mutex *, int *);
    FVM_API unsigned long fvm_mutex_origin(FVM_Mutex *);
    FVM_API unsigned long fvm_mutex_identity(FVM_Mutex *);
    FVM_API FVM_Result fvm_extensions_wrap_mutex(FVM_Context *, FVM_Mutex *, FVM_Object **);
#ifdef __cplusplus
}
#endif
#endif
