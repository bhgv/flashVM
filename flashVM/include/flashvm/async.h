/* Opt-in frame-free callbacks, executed by the context's owning event loop. */
#ifndef FLASHVM_ASYNC_H
#define FLASHVM_ASYNC_H /* Guard the C89 asynchronous scheduling extension. */
#include "value.h"
#ifdef __cplusplus
extern "C" {
#endif


/* Install Async.post(callback,...args) and Async.cancel(id), preserving existing
 * own globals (a conflict returns INVALID_ARGUMENT). Explicitly opt in per context;
 * no legacy SWF version gate. Owner-thread only, not during GC/destruction.
 * Native method graph serialization is unsupported. No callbacks run here. */
FVM_API FVM_Result fvm_async_install(FVM_Context *);


/* Queue a same-context callable and up to256 borrowed arguments, copied into
 * strong roots until consumption/cancellation. id is required and unchanged on
 * failure. Bounded queue/IDs and OOM/work/clock failures follow timers.h. The
 * callback runs on a later fvm_timers_poll, even without advancing a SWF frame
 * or the sampled clock. Posts during delivery wait for the following poll.
 * Owner-thread only; no worker thread, inline recursion or preemption. */
FVM_API FVM_Result fvm_async_post(FVM_Context *, FVM_Object *, unsigned int,
                                 const FVM_Value *, unsigned long *id);


/* Cancel a shared context-local scheduling ID. Missing/zero IDs are harmless;
 * values beyond the exact scheduler ID domain reject. No callback runs here.
 * Owner-thread/lifecycle restrictions match posting and timers.h. */
FVM_API FVM_Result fvm_async_cancel(FVM_Context *, unsigned long id);

#ifdef __cplusplus
}
#endif
#endif
