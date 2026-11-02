/* Opt-in two-owner managed scheduling; all heap turns remain exclusive. */
#ifndef DVM_MANAGED_WORKERS_H
#define DVM_MANAGED_WORKERS_H


/* Start one extra OS executor after GC/world startup. turn runs exactly one
 * shared-heap scheduler turn on its caller: positive progress, zero idle,
 * negative fatal managed failure. No managed pointer crosses this interface.
 * Start never waits for heap admission while controller owns the heap. */
int dvm_managed_workers_start(int (*turn)(void));


/* Yield an empty controller turn through FIFO heap admission. May wait for
 * another executor or collector. Return zero for a worker/protocol failure.
 * idle requests a bounded native sleep OUTSIDE the heap before reentry.
 * No current Prog, native token or C-held managed root may be live here. */
int dvm_managed_workers_yield(int idle);


/* Count one controller scheduler turn, only when the optional service is live. */
void dvm_managed_workers_record(void);


/* Called on an ACTIVE heap owner before ordinary FVM_PortFile I/O. Return one
 * while the pool is enabled: cursor-bearing file operations must keep the heap
 * until a separate per-handle concurrency audit. Standard streams stay releasable.
 * Counts actual guarded calls under heap exclusion; no handle is retained. */
int dvm_managed_workers_serialize_file(void);


/* Authorize only the live helper's ACTIVE claimed turn for copied UI RPC.
 * Never authorizes UI main, foreign TLS or a native waiter; no root is returned.
 * Requires the caller's current phase check before reading shared heap identity. */
int dvm_managed_workers_current(void);


/* Stop and join the helper BEFORE collector shutdown. Controller temporarily
 * leaves its empty heap turn so helper entry/reentry/detach can finish, then
 * reacquires it. Native OS I/O cancellation is cooperative, not preemptive.
 * diagnostics prints native counters after join. No-op when not started. */
int dvm_managed_workers_stop(int diagnostics);
#endif
