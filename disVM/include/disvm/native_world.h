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


/* Allocate an empty domain. Destroy succeeds only with no participants/collector. */
DVM_NativeWorld *dvm_world_create(void);


int dvm_world_destroy(DVM_NativeWorld *world);


/* Register the currently bound context and install its instruction-boundary poll.
 * Fails during collection or if another poll owner already uses the context. */
DVM_WorldParticipant *dvm_world_join(DVM_NativeWorld *world, DVM_NativeContext *context);


/* Remove an inactive root set on its owner. Fails while collection is requested. */
int dvm_world_detach(DVM_WorldParticipant *participant);


/* Enter mutator execution, waiting for a pending collection to finish. */
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
