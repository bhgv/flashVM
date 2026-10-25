/* FlashVM: explicit owning-thread delivery of AS1/AS2 scheduled callbacks. */
#ifndef FLASHVM_TIMERS_H
#define FLASHVM_TIMERS_H /* Guard the C89 timer-poll API. */
#include "flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif


/* Poll the existing monotonic context clock and attempt each due entry once in registration order.
 * Install setInterval/clearInterval (SWF6+) and setTimeout/clearTimeout (SWF8+) with standard.h.
 * No sleeping, system thread or automatic movie advancement; call on the context owner thread.
 * delivered is required: valid calls set it to zero, then count attempted callbacks, including a failing one.
 * Before delivery errors preserve pending entries; afterwards completed consumption/rearming stays committed.
 * New registrations wait for another poll; cancellation suppresses later captured IDs. Late intervals
 * rearm from this poll's clock sample, without catch-up bursts. Callback errors stop this poll.
 * [GC:] Pending callbacks and copied arguments are strong roots; active snapshots survive self-cancellation.
 * Reentrant polling, GC/finalization/destruction phases are INVALID_ARGUMENT and preserve delivered.
 * OOM, execution-work cancellation/limit and clock errors propagate. No arbitrary C callback preemption.
 * See doc/TIMERS.md for bounded delay/argument/ID limits and unaccepted original-player behavior. */
FVM_API FVM_Result fvm_timers_poll(FVM_Context *, unsigned int *delivered);


/* Query the nearest pending deadline without delivering, allocating, sleeping or changing the queue.
 * max_wait_ms is the host's ceiling (including zero); wait_ms and pending are required outputs.
 * Empty queues return max_wait_ms and pending=0 without reading the clock. Otherwise pending=1 and
 * wait_ms is min(max_wait_ms, ceil(nonnegative remaining milliseconds)); due entries return zero.
 * Call on the owner thread; a query during a callback is allowed and shares its execution budget.
 * Clock/work/cancellation failures preserve both outputs; lifecycle guards match polling.
 * Clock sampling can advance the context's retained elapsed sample even on a later work error.
 * This is a hint: process host events, query again after queue changes, and call poll when awake.
 * [GC:] The query invokes no user code and retains no new values or borrowed pointers. */
FVM_API FVM_Result fvm_timers_next_delay(FVM_Context *, unsigned long max_wait_ms,
                                         unsigned long *wait_ms, int *pending);
#ifdef __cplusplus
}
#endif
#endif
