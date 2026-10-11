#ifndef FLASHVM_WORKER_H
#define FLASHVM_WORKER_H
#include "value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Worker owns an independent runtime and context. Only primitive values
 * (including copied UTF-8 strings) may cross the thread boundary.
 * Worker callbacks execute on a native OS thread. */
typedef struct FVM_Worker FVM_Worker;
typedef FVM_Result (*FVM_WorkerFunction)(FVM_Context *, const FVM_Value *, FVM_Value *, void *);
/* Starts immediately. Caller must join before destroying the handle.
 * userdata lifetime must extend until join; it is not copied. */
FVM_API FVM_Result fvm_worker_run(const FVM_Config *, FVM_WorkerFunction,
                                  const FVM_Value *, void *, FVM_Worker **);
/* Waits for completion; returned strings remain valid until destroy. */
FVM_API FVM_Result fvm_worker_join(FVM_Worker *, FVM_Value *);
/* Must only be called after join. */
FVM_API void fvm_worker_destroy(FVM_Worker *);
#ifdef __cplusplus
}
#endif
#endif
