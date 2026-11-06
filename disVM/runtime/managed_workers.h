/* Opt-in fixed/elastic managed scheduling; all heap turns remain exclusive. */
#ifndef DVM_MANAGED_WORKERS_H
#define DVM_MANAGED_WORKERS_H



enum
{
    DVM_MANAGED_TURN_IDLE,
    DVM_MANAGED_TURN_GUEST,
    DVM_MANAGED_TURN_MAINTENANCE
};


/* Start count-1 extra OS executors after GC/world startup.
 * count selects at least one initial executor. automatic grows on native waits
 * and retires idle helpers; limit zero is uncapped, diagnostics emits --stats.
 * turn runs one
 * shared-heap scheduler turn on its caller: positive progress, zero idle,
 * negative fatal managed failure. cancel_waits wakes native waits on failure
 * without taking heap ownership. No managed pointer crosses this interface.
 * Return one when all helpers were created; zero on invalid input or after
 * joining every partially started helper and restoring controller ownership.
 * Requires the controller's empty ACTIVE turn. No startup wait holds the heap. */
int dvm_managed_workers_start(int count, int automatic, unsigned long limit, int diagnostics,
    int (*turn)(void), void (*cancel_waits)(void));


/* Dedicated guest lifecycle. Call under exclusive heap admission. */
struct Prog;
int dvm_managed_workers_start_guest(struct Prog *guest);
struct Prog *dvm_managed_workers_guest(void);
void dvm_managed_workers_guest_retired(int pid);

/* Compensate an impending audited native suspension while still ACTIVE.
 * Return a stable diagnostic if compensation is refused, or NULL on success. */
const char *dvm_managed_workers_native_wait(void);


/* Mark/consume native completion queued for heap reentry. arriving selects1/0;
 * returns one when tracked. Native mutex only; never borrows managed data. */
int dvm_managed_workers_returning(int arriving);


/* Record committed native withdrawal for automatic --stats lifecycle evidence.
 * Requires ACTIVE heap ownership; does not change scheduling or roots. */
void dvm_managed_workers_native_suspended(void);


/* Signal runnable work or a pending native request. The caller must own
 * NativeWorld or otherwise guarantee service lifetime through the signal. */
void dvm_managed_workers_wake(void);


/* Yield an empty controller turn through FIFO heap admission. May wait for
 * another executor or collector. Return zero for a worker/protocol failure.
 * idle requests a bounded native sleep OUTSIDE the heap before reentry.
 * No current Prog, native token or C-held managed root may be live here. */
int dvm_managed_workers_yield(int idle);


/* Park a rootless non-UI controller awaiting logical channel work. Heap is
 * released; pending checks native requests after the wake epoch is captured.
 * The callback is invoked without the scheduler mutex. */
int dvm_managed_workers_park(int (*pending)(void));


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
