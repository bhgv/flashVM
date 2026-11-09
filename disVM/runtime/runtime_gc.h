/* Production RC and cycle collection behind native world rendezvous. */
#ifndef DVM_RUNTIME_GC_H
#define DVM_RUNTIME_GC_H

/* [GC:] Start Fast/Cycles collectors on the current bound mutator. locked_poll
 * selects mutex reference polling. Returns zero on failure.
 * The caller must keep all heap work inside the resulting active interval. */
struct Prog;
int dvm_runtime_gc_start(int locked_poll);

/* EN: Publish a complete process, borrow the root head, or withdraw a process.
 * RU: Опубликовать готовый процесс, получить голову корней или снять процесс. */
void dvm_runtime_process_root_add(struct Prog *process);
struct Prog *dvm_runtime_process_roots(void);
int dvm_runtime_process_root_remove(struct Prog *process);

extern int dvm_runtime_event_cycles;
extern int dvm_runtime_disable_cycles;
extern int dvm_runtime_idle_cycles;
void dvm_runtime_gc_owner_lost(void);

/* [GC:] Coalesce one real rungc pass. Called by the mutator between turns;
 * returns zero after native failure. Does not wait for collection completion. */
int dvm_runtime_gc_request(void);

/* EN: Notify Fast of published native retirement work without admission.
 * RU: Уведомить Fast о опубликованном задании без допуска к куче. */
void dvm_runtime_gc_notify_fast(void);

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
 * -1 if guards are held or the pool refuses, with ownership unchanged. Protocol violations
 * terminate before unsafe heap access. */
int dvm_runtime_gc_native_try_leave(void);

/* [GC:] Reenter after the native operation, before touching any managed memory.
 * released is exactly the matching leave result. No nesting/migration is allowed. Pairing and epoch snapshots live in this
 * TLS context; shared collector epochs are sampled only after reentry. */
void dvm_runtime_gc_native_enter(int released);

/* Publish the continuation and enter shared admission. A positive result
 * requires shared_finish after releasing every guard. */
int dvm_runtime_gc_shared_try_enter(void);
/* Publish changed registers after the instruction is complete and unguarded. */
void dvm_runtime_gc_shared_publish(void);
void dvm_runtime_gc_shared_poll(void);
int dvm_runtime_gc_shared_has_exclusive_waiters(void);
/* Park a shared guest with its current published roots, then reenter shared. */
void dvm_runtime_gc_shared_drain(void);
void dvm_runtime_gc_shared_finish(void);

/* [GC:] Leave heap execution, drain/join both collectors, remove the poll hook and
 * destroy coordination. Called on the mutator on every normal/error return.
 * Returns zero if a worker/protocol failed; optional diagnostics follow join. */
int dvm_runtime_gc_stop(int diagnostics);
#endif
