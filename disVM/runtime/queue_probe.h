/* Optional actual native-owner observer; caller includes matching interp.h. */
#ifndef DVM_RUNTIME_QUEUE_PROBE_H
#define DVM_RUNTIME_QUEUE_PROBE_H
#include "disvm/native_context.h"
extern int dvm_queue_probe_requested; /* Immutable launch option until every probe has joined. */

/* Enable before publishing the Dis worker. Returns zero on native allocation. */
int dvm_queue_probe_enable(void);


/* [GC:] Called only after audited world leave with immutable rooted process.
 * owner/roots are borrowed until synchronous observer join. The observer uses
 * real runtime attach/detach and examines managed state only under its heap turn.
 * Failure is process-fatal; never return to managed code with a broken lease. */
void dvm_queue_probe_run(Prog *process, DVM_NativeContext *owner, Prog **roots);


/* Print native observer/join counters after all calls, for explicit --stats. */
void dvm_queue_probe_statistics(void);


/* Destroy native diagnostics only after every synchronous probe has joined. */
void dvm_queue_probe_finish(void);
#endif
