/* Cooperative GC safepoints for native Dis mutators, independent of either heap. */
#ifndef DISVM_NATIVE_WORLD_H
#define DISVM_NATIVE_WORLD_H
#include "native_context.h"

#ifdef __cplusplus
extern "C"
{
#endif
/* One coordination domain; it does not allocate, mark or reclaim a VM heap. */
typedef struct DVM_NativeWorld DVM_NativeWorld;
/* A registered native owner whose roots are published before becoming inactive. */
typedef struct DVM_WorldParticipant DVM_WorldParticipant;


/* [GC:] Borrowed root snapshot, valid only inside the stopped-world visitor. */
typedef void (*DVM_WorldRoots)(
    DVM_NativeContext *context, void *registers, size_t register_size, void *process, void *user);


/* Mutex-protected observation, not a reservation or a polling synchronization rule. */
typedef struct DVM_WorldState
{
    size_t participants; /* Registered root sets, active or inactive. */
    size_t active;       /* Native mutators between enter and leave. */
    size_t parked;       /* Active mutators waiting at a safepoint. */
    int stopping;        /* Collector requested a rendezvous. */
    int stopped;         /* All active mutators have parked. */
} DVM_WorldState;


/* Mutex-protected admission observation, never permission to mutate. */
typedef struct DVM_WorldAdmission
{
    int serial;          /* Nonzero selects coordinated hosted GC admission. */
    size_t waiting;      /* Legacy exclusive-queue counter, always zero. */
    size_t admissions;   /* Successful entries and promotions; counter may wrap. */
    size_t contentions;  /* Legacy exclusive-queue counter, always zero. */
} DVM_WorldAdmission;


/* Allocate a hosted GC admission domain; locked_poll selects the reference
 * instruction poll. NULL reports allocation failure. Enter/leave are heap-turn
 * boundaries; no thread may cancel while waiting. Inactive roots stay immutable
 * through reentry/detach. This does not retrofit synchronization into a VM's
 * scheduler or publish otherwise hidden native C-stack roots. */
DVM_NativeWorld *dvm_world_create_serial(int locked_poll);


/* Copy live native admission counters under the mutex; invalid arguments fail.
 * out is caller-owned. This snapshot does not reserve an admission or root scan. */
int dvm_world_admission(DVM_NativeWorld *world, DVM_WorldAdmission *out);

/* Observe any admitted guest, excluding shared collector participants. */
int dvm_world_has_active_guest(DVM_NativeWorld *world);


/* Legacy exclusive waiter query. Always returns zero. */
size_t dvm_world_exclusive_waiting(DVM_NativeWorld *world);

/* Return peak SHARED occupancy of a borrowed live world, or zero for NULL. */
size_t dvm_world_shared_peak(DVM_NativeWorld *world);


/* Allocate a parallel-admission domain. The caller supplies synchronization for
 * shared heap mutations; this constructor only coordinates root stops. Destroy
 * succeeds only with no participants/collector. */
DVM_NativeWorld *dvm_world_create(void);


/* Create an immutable polling mode: nonzero retains the fully locked reference
 * route; zero uses an atomic request check when the compiler supports it. */
DVM_NativeWorld *dvm_world_create_mode(int locked_poll);


/* Copy current owner's counters without shared counter reads or heap access.
 * Returns zero for foreign/unbound participants or null output addresses. */
int dvm_world_owner_polls(DVM_WorldParticipant *participant, size_t *fast, size_t *locked);


int dvm_world_destroy(DVM_NativeWorld *world);


/* Register the currently bound context and install its instruction-boundary poll.
 * Fails during collection or if another poll owner already uses the context. */
DVM_WorldParticipant *dvm_world_join(DVM_NativeWorld *world, DVM_NativeContext *context);

/* Collector registration for nonblocking shared turns only. */
DVM_WorldParticipant *dvm_world_join_service(DVM_NativeWorld *world, DVM_NativeContext *context);


/* Remove an inactive root set on its owner. Fails while collection is requested. */
int dvm_world_detach(DVM_WorldParticipant *participant);


/* Close new registrations atomically only when participant is the current last
 * serialized owner (active or idle), apart from idle collector services without borrows. Existing
 * root operations remain usable until the remaining threads join. */
int dvm_world_seal_last_owner(DVM_WorldParticipant *participant);


/* Enter mutator execution after any pending collection. Other mutators can
 * remain active. Double and foreign entry fail without changing the world. */
int dvm_world_enter(DVM_WorldParticipant *participant);


/* Enter an audited shared interval. The caller protects the accessed graph
 * and finishes or polls before collection. Use the same poll and leave API. */
int dvm_world_enter_shared(DVM_WorldParticipant *participant);

/* Attempt shared admission without waiting for a pending GC barrier. */
int dvm_world_try_enter_shared(DVM_WorldParticipant *participant);

/* Convert an active mutator to SHARED if the GC barrier is open. */
int dvm_world_try_share(DVM_WorldParticipant *participant);

/* Return to ordinary mutation. Publish roots before waiting for pending GC. */
int dvm_world_promote_shared(DVM_WorldParticipant *participant);


/* [GC:] Publish stable roots before blocking/I/O or detaching. No heap mutation
 * may occur outside an active interval; process/register storage stays alive. */
int dvm_world_leave(DVM_WorldParticipant *participant);


/* Collector must be a bound context distinct from every participant in this world.
 * Begin waits until every active mutator polls/leaves. No forced suspension.
 * Competing/nested collectors fail. Host must pair a successful begin with end. */
int dvm_world_stop_begin(DVM_NativeWorld *world, DVM_NativeContext *collector);

/* Claim only an already idle world. Pair success with stop_end. */
int dvm_world_try_quiescent(DVM_NativeWorld *world, DVM_NativeContext *collector);

/* Wait for a heap transition or a short timeout when admission is busy. */
void dvm_world_wait_quiescent_retry(DVM_NativeWorld *world);


/* [GC:] Visit stable roots under the world mutex. Callback must not reenter world
 * APIs, block, throw/longjmp or free context/register/process storage. */
int dvm_world_visit(DVM_NativeWorld *world, DVM_WorldRoots visitor, void *user);


/* Resume mutators; only the collecting native context may end the stop. */
int dvm_world_stop_end(DVM_NativeWorld *world);


/* Observe counters without waiting for the stop protocol to complete. */
int dvm_world_state(DVM_NativeWorld *world, DVM_WorldState *out);


/* [GC:] Return nonzero only for this exact bound stopped-world collector, with
 * no held object guards. world borrowed/live; no root traversal or allocation.
 * Collector retains its own stop until stop_end; this is not mutator admission. */
int dvm_world_collector_owns(DVM_NativeWorld *world);


#ifdef __cplusplus
}
#endif
/* Announce a bounded raw-pointer borrow before its first load. Owner-only,
 * nestable and independent of ordinary admission. End after the last use. */
int dvm_world_read_begin(DVM_WorldParticipant *participant);
int dvm_world_read_end(DVM_WorldParticipant *participant);

/* Observe announced pointer borrows under the world mutex. */
int dvm_world_has_borrowers(DVM_NativeWorld *world);

/* After unlinking a retired object, issue its epoch. Zero disables early
 * reclamation. A positive observation covers old admission intervals and explicit borrows.
 * Pointers retained outside admission require an explicit borrow. */
unsigned long dvm_world_retire_epoch(DVM_NativeWorld *world);
int dvm_world_epoch_elapsed(DVM_NativeWorld *world, unsigned long epoch);

#endif
