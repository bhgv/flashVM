/* Deferred RC ownership inside the production stopped-world heap protocol. */
#ifndef DVM_RUNTIME_RC_H
#define DVM_RUNTIME_RC_H

/* [GC:] Enable an empty bounded queue on the current Dis mutator. Called only
 * after collector startup, before any collection request. No ownership transfer. Native metadata
 * allocation failure selects inline RC; previous session must be finished. */
void dvm_runtime_rc_start(void);

struct Type;
/* Transfer mapped owners and optional displaced reference under shared admission.
 * Return 1 on success, -1 when draining can make room, or 0 on other refusal.
 * Refusal leaves the frame and module owners untouched. */
int dvm_runtime_rc_transfer_frame(const void *frame, const struct Type *type,
    void *extra);

/* [GC:] Transfer the caller's last reference into the native queue, retaining
 * one queue-owned reference and an external GC root. Called by generated destroy
 * after its normal header check. Returns one when transferred, zero to retain
 * inline destruction. Only an ACTIVE executor of the owning runtime world is
 * admitted; native/idle/foreign/collector contexts refuse before payload reads.
 * Never queues collector-side recursive destruction. */
int dvm_runtime_rc_defer(void *value);

/* [GC:] Release at most budget queue owners with the retained destroy algorithm.
 * Caller is the collector with the world stopped. Returns remaining owners. */
unsigned int dvm_runtime_rc_drain(unsigned int budget);

/* [GC:] Query queue size only on the active Dis owner, before releasing the
 * world interval, including helper executors. Returns zero for other roles;
 * may wait for native metadata mutex, never managed admission or allocation.
 * Source access, statistics and caller phase still require world admission. */
unsigned int dvm_runtime_rc_pending(void);

/* [GC:] Stop admission on the mutator before leaving its heap interval. */
void dvm_runtime_rc_close(void);

/* [GC:] Report/reset after collector join. Returns zero if owners remain. */
int dvm_runtime_rc_finish(int diagnostics);
#endif
