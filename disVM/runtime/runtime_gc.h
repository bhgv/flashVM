/* Production RC and cycle collection behind native world rendezvous. */
#ifndef DVM_RUNTIME_GC_H
#define DVM_RUNTIME_GC_H

/* [GC:] Start Fast/Cycles collectors on the current bound mutator. locked_poll selects the mutex reference route; roots addresses the
 * runtime's Prog list and remains owned/alive until stop. Returns zero on failure.
 * The caller must keep all heap work inside the resulting active interval. */
int dvm_runtime_gc_start(Prog **roots, int locked_poll);

/* [GC:] Coalesce one real rungc pass. Called by the mutator between turns;
 * returns zero after native failure. Does not wait for collection completion. */
int dvm_runtime_gc_request(void);

/* [GC:] Leave heap execution, drain/join both collectors, remove the poll hook and
 * destroy coordination. Called on the mutator on every normal/error return.
 * Returns zero if a worker/protocol failed; optional diagnostics follow join. */
int dvm_runtime_gc_stop(int diagnostics);
#endif
