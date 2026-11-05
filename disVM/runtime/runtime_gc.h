/* Production RC and cycle collection behind native world rendezvous. */
#ifndef DVM_RUNTIME_GC_H
#define DVM_RUNTIME_GC_H

/* [GC:] Start Fast/Cycles collectors on the current bound mutator. locked_poll
 * selects mutex reference polling; roots addresses the runtime Prog list and
 * remains owned/alive until stop. Returns zero on failure.
 * The caller must keep all heap work inside the resulting active interval. */
int dvm_runtime_gc_start(Prog **roots, int locked_poll);

/* [GC:] Coalesce one real rungc pass. Called by the mutator between turns;
 * returns zero after native failure. Does not wait for collection completion. */
int dvm_runtime_gc_request(void);

/* [GC:] Register a newly bound native owner with empty process slots and enter
 * its heap turn. Host publishes the live service first and joins all registration
 * attempts/owners before stop. Zero refuses; no managed executor is created. */
int dvm_runtime_gc_attach(void);

/* [GC:] Leave/detach an extra active or idle owner with empty process slots. The original
 * controller retires through stop instead. Returns zero for outstanding calls,
 * wrong ownership/phase or collector conflict; TLS stays bound through the call.
 * An IDLE refusal can be retried after the collector finishes, without leaving
 * the heap twice. Host lifetime/join requirements still apply. */
int dvm_runtime_gc_detach(void);

/* [GC:] Temporarily leave heap execution for an audited native callback whose
 * C locals contain no unregistered managed owners. Publishes the complete native
 * call REG/stack roots first. Returns one when released, zero in inline mode or
 * protected GC regions or retained synchronous Native64Caller holds. Returns
 * -1 on pool resource refusal with ownership unchanged. Protocol violations
 * terminate before unsafe heap access. */
int dvm_runtime_gc_native_try_leave(void);

/* [GC:] Reenter after the native operation, before touching any managed memory.
 * released is exactly the matching leave result. No nesting/migration is allowed. Pairing and epoch snapshots live in this
 * TLS context; shared collector epochs are sampled only after reentry. */
void dvm_runtime_gc_native_enter(int released);

/* [GC:] Leave heap execution, drain/join both collectors, remove the poll hook and
 * destroy coordination. Called on the mutator on every normal/error return.
 * Returns zero if a worker/protocol failed; optional diagnostics follow join. */
int dvm_runtime_gc_stop(int diagnostics);
#endif
