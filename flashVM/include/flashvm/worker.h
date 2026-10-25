/* FlashVM: Public embedding API declarations: worker.
 * File: include/flashvm/worker.h.
 */

#ifndef FLASHVM_WORKER_H
/* Define FLASHVM WORKER H constant or compilation switch. */
#define FLASHVM_WORKER_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Worker owns an independent runtime and context. Only primitive values
     * (including copied UTF-8 strings) may cross the thread boundary.
     * Worker callbacks execute on a native OS thread. */
typedef struct FVM_Worker FVM_Worker;


typedef FVM_Result (*FVM_WorkerFunction)(FVM_Context *, const FVM_Value *, FVM_Value *, void *);
/* Starts immediately. Caller must join before destroying the handle.
     * userdata lifetime must extend until join; it is not copied. */


/* Run worker. */
FVM_API FVM_Result fvm_worker_run(const FVM_Config *, FVM_WorkerFunction, const FVM_Value *,
                                  void *, FVM_Worker **);
/* Creates an OS thread which waits until fvm_worker_start() is called.
     * A suspended worker can be joined; join automatically starts it to avoid deadlock. */


/* Run worker suspended. */
FVM_API FVM_Result fvm_worker_run_suspended(const FVM_Config *, FVM_WorkerFunction,
                                            const FVM_Value *, void *, FVM_Worker **);
/* Idempotent: a running or completed worker returns FVM_OK. */


/* Implement worker start for this subsystem. */
FVM_API FVM_Result fvm_worker_start(FVM_Worker *);


/* Define FVM_WorkerState enum and its representation. */
typedef enum FVM_WorkerState
{
    FVM_WORKER_SUSPENDED = 0 /* Worker is suspended. */,
    FVM_WORKER_RUNNING = 1 /* Worker is running. */,
    FVM_WORKER_FINISHED = 2 /* Worker is finished. */,
    FVM_WORKER_JOINED = 3 /* Worker is joined. */
} FVM_WorkerState;

/* Thread-safe snapshot of lifecycle state. */


/* Implement worker state for this subsystem. */
FVM_API FVM_Result fvm_worker_state(FVM_Worker *, FVM_WorkerState *);
/* Concurrent joins are serialized. Self-join returns FVM_INVALID_ARGUMENT.
       Destroy must not race any operation. Returned strings live until destroy. */


/* Join worker. */
FVM_API FVM_Result fvm_worker_join(FVM_Worker *, FVM_Value *);
/* Cooperative cancellation request. Never forcibly terminates an OS thread.
     * Callback may poll fvm_worker_stop_requested() using its worker handle.
     * For safe handle publication, use run_suspended(), then start().
     * A suspended worker is started when stop is requested so it can exit.
     */


/* Implement worker stop for this subsystem. */
FVM_API FVM_Result fvm_worker_stop(FVM_Worker *worker);


/* Implement worker stop requested for this subsystem. */
FVM_API FVM_Result fvm_worker_stop_requested(FVM_Worker *worker, int *requested);
/* Wait for all workers. Null entries and duplicate handles are rejected.
     * Results, if supplied, remain owned by their workers until destroy.
     * Joins all entries even if an individual callback reports an error.
     */


/* Join worker all. */
FVM_API FVM_Result fvm_worker_join_all(FVM_Worker **workers, unsigned int count,
                                       FVM_Value *results);
/* Borrow current context worker; FVM_NOT_FOUND in a host context.
       The returned handle remains owned by its creator. */


/* Implement worker current for this subsystem. */
FVM_API FVM_Result fvm_worker_current(FVM_Context *, FVM_Worker **);
/* Nonblocking result snapshot; FVM_NOT_FOUND while still running. */


/* Implement worker result for this subsystem. */
FVM_API FVM_Result fvm_worker_result(FVM_Worker *, FVM_Value *);
/* Must only be called after join. */


/* Destroy worker. */
FVM_API void fvm_worker_destroy(FVM_Worker *);
#ifdef __cplusplus
}
#endif
#endif
