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
/* Creates an OS thread which waits until fvm_worker_start() is called.
 * A suspended worker can be joined; join automatically starts it to avoid deadlock. */
FVM_API FVM_Result fvm_worker_run_suspended(const FVM_Config *, FVM_WorkerFunction,
                                            const FVM_Value *, void *, FVM_Worker **);
/* Idempotent: a running or completed worker returns FVM_OK. */
FVM_API FVM_Result fvm_worker_start(FVM_Worker *);
typedef enum FVM_WorkerState {
    FVM_WORKER_SUSPENDED = 0,
    FVM_WORKER_RUNNING = 1,
    FVM_WORKER_FINISHED = 2,
    FVM_WORKER_JOINED = 3
} FVM_WorkerState;
/* Thread-safe snapshot of lifecycle state. */
FVM_API FVM_Result fvm_worker_state(FVM_Worker *, FVM_WorkerState *);
/* Concurrent joins are serialized. Self-join returns FVM_INVALID_ARGUMENT.
   Destroy must not race any operation. Returned strings live until destroy. */
FVM_API FVM_Result fvm_worker_join(FVM_Worker *, FVM_Value *);
/* Cooperative cancellation request. Never forcibly terminates an OS thread.
 * Callback may poll fvm_worker_stop_requested() using its worker handle.
 * For safe handle publication, use run_suspended(), then start().
 * A suspended worker is started when stop is requested so it can exit.
 */
FVM_API FVM_Result fvm_worker_stop(FVM_Worker *worker);
FVM_API FVM_Result fvm_worker_stop_requested(FVM_Worker *worker, int *requested);
/* Wait for all workers. Null entries and duplicate handles are rejected.
 * Results, if supplied, remain owned by their workers until destroy.
 * Joins all entries even if an individual callback reports an error.
 */
FVM_API FVM_Result fvm_worker_join_all(FVM_Worker **workers,
                                       unsigned int count, FVM_Value *results);
/* Borrow current context worker; FVM_NOT_FOUND in a host context.
   The returned handle remains owned by its creator. */
FVM_API FVM_Result fvm_worker_current(FVM_Context *,FVM_Worker **);
/* Nonblocking result snapshot; FVM_NOT_FOUND while still running. */
FVM_API FVM_Result fvm_worker_result(FVM_Worker *, FVM_Value *);
/* Must only be called after join. */
FVM_API void fvm_worker_destroy(FVM_Worker *);
#ifdef __cplusplus
}
#endif
#endif
