/* Opt-in bounded managed scheduling; all heap turns remain exclusive. */
#ifndef DVM_MANAGED_WORKERS_H
#define DVM_MANAGED_WORKERS_H

#define DVM_MANAGED_EXECUTORS_MAX 8 /* Explicit total OS-executor cap, including controller. */


/* Start count-1 extra OS executors after GC/world startup.
 * count selects 2..DVM_MANAGED_EXECUTORS_MAX total executors. turn runs one
 * shared-heap scheduler turn on its caller: positive progress, zero idle,
 * negative fatal managed failure. No managed pointer crosses this interface.
 * Return one when all helpers were created; zero on invalid input or after
 * joining every partially started helper and restoring controller ownership.
 * Requires the controller's empty ACTIVE turn. No startup wait holds the heap. */
int dvm_managed_workers_start(int count, int (*turn)(void));


/* Yield an empty controller turn through FIFO heap admission. May wait for
 * another executor or collector. Return zero for a worker/protocol failure.
 * idle requests a bounded native sleep OUTSIDE the heap before reentry.
 * No current Prog, native token or C-held managed root may be live here. */
int dvm_managed_workers_yield(int idle);


/* Count one controller scheduler turn, only when the optional service is live. */
void dvm_managed_workers_record(void);


/* Count one ordinary file operation on the ACTIVE heap owner. No handle/root is
 * retained; the separate per-handle FIFO serializes native cursor access. */
void dvm_managed_workers_file_call(void);


/* Authorize only the live helper's ACTIVE claimed turn for copied UI RPC.
 * Never authorizes UI main, foreign TLS or a native waiter; no root is returned.
 * Requires the caller's current phase check before reading shared heap identity. */
int dvm_managed_workers_current(void);


/* Stop and join all started helpers BEFORE collector shutdown. Controller temporarily
 * leaves its empty heap turn so every helper can enter/reenter/detach, then
 * reacquires it. Native OS I/O cancellation is cooperative, not preemptive.
 * diagnostics prints native counters after join. No-op when not started. */
int dvm_managed_workers_stop(int diagnostics);
#endif
