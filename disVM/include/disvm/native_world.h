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


/* Mutex-protected FIFO admission observation, never permission to mutate. */
typedef struct DVM_WorldAdmission
{
    int serial;          /* Nonzero means only one heap-mutator interval is admitted. */
    size_t waiting;      /* Queued inactive native owners with stable published roots. */
    size_t admissions;   /* Successful serialized entries; unsigned counter may wrap. */
    size_t contentions;  /* Entries which could not initially acquire their turn. */
} DVM_WorldAdmission;


/* Allocate an exclusive FIFO mutator domain; locked_poll selects the reference
 * instruction poll. NULL reports allocation failure. Enter/leave are heap-turn
 * boundaries; no thread may cancel while waiting. Inactive roots stay immutable
 * through reentry/detach. This does not retrofit synchronization into a VM's
 * scheduler or publish otherwise hidden native C-stack roots. */
DVM_NativeWorld *dvm_world_create_serial(int locked_poll);


/* Copy live native admission counters under the mutex; invalid arguments fail.
 * out is caller-owned. This snapshot does not reserve an admission or root scan. */
int dvm_world_admission(DVM_NativeWorld *world, DVM_WorldAdmission *out);


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


/* Remove an inactive root set on its owner. Fails while collection is requested. */
int dvm_world_detach(DVM_WorldParticipant *participant);


/* Enter mutator execution after collection; a serialized world also waits for
 * its FIFO heap turn. Double/foreign entry fails without consuming a ticket. */
int dvm_world_enter(DVM_WorldParticipant *participant);


/* [GC:] Publish stable roots before blocking/I/O or detaching. No heap mutation
 * may occur outside an active interval; process/register storage stays alive. */
int dvm_world_leave(DVM_WorldParticipant *participant);


/* Collector must be a bound context distinct from every participant in this world.
 * Begin waits until every active mutator polls/leaves. No forced suspension.
 * Competing/nested collectors fail. Host must pair a successful begin with end. */
int dvm_world_stop_begin(DVM_NativeWorld *world, DVM_NativeContext *collector);


/* [GC:] Visit stable roots under the world mutex. Callback must not reenter world
 * APIs, block, throw/longjmp or free context/register/process storage. */
int dvm_world_visit(DVM_NativeWorld *world, DVM_WorldRoots visitor, void *user);


/* Resume mutators; only the collecting native context may end the stop. */
int dvm_world_stop_end(DVM_NativeWorld *world);


/* Observe counters without waiting for the stop protocol to complete. */
int dvm_world_state(DVM_NativeWorld *world, DVM_WorldState *out);
#ifdef __cplusplus
}
#endif
#endif
