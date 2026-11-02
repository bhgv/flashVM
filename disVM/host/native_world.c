/* Native root rendezvous, with optional FIFO exclusive heap-mutator admission. */
#include "disvm/native_world.h"
#include "channel_sync.h"
#include "world_request.h"
#include <stdlib.h>


/* [GC:] Registered roots stay alive through detach. Only the owner mutates active
 * register/process contents; collector reads published pointers after parking. */
struct DVM_WorldParticipant
{
    DVM_NativeWorld *world;     /* Borrowed coordination domain. */
    DVM_NativeContext *context; /* Bound owner; kept alive until detach. */
    DVM_WorldParticipant *next; /* Membership under the world mutex. */
    void *registers;            /* Stable address inside extended up. */
    size_t register_size;       /* Native byte extent for the visitor. */
    void *process;              /* Root value published at leave/poll. */
    size_t fast_polls;           /* Owner-only count, never read by the collector. */
    size_t locked_polls;         /* Owner-only slow-path count. */
    int active;                 /* Owner may execute mutator instructions. */
    int parked;                 /* Owner is currently waiting for resume. */
};


/* [GC:] Mutex protects membership, root publication and stop predicates only.
 * It is never held while executing a normal mutator instruction. */
struct DVM_NativeWorld
{
    FVM_PortSync *sync;                 /* Guards state, never instruction bodies. */
    DVM_WorldParticipant *participants; /* Registered roots, including inactive ones. */
    DVM_NativeContext *collector;       /* Bound collecting owner, or null. */
    size_t count;                       /* Number of root sets. */
    size_t active;                      /* Mutators between enter and leave. */
    size_t parked;                      /* Active mutators stopped at poll. */
    long request;                       /* Atomic stop request, not root publication. */
    int sealed;                        /* Registration closed by the last active owner before shutdown. */
    int serial;                        /* Immutable exclusive-mutator mode; not a parallel heap barrier. */
    size_t next_ticket;                /* Next FIFO admission ticket under sync; unsigned wrap is intentional. */
    size_t serving_ticket;             /* Oldest queued admission; advances only on a successful enter. */
    size_t waiting;                    /* Inactive owners queued for admission, including a ready ticket. */
    size_t admissions;                 /* Successful serialized entries under sync. */
    size_t contentions;                /* Entries that observed another owner, collector or older waiter. */
    int locked_poll;                    /* Immutable reference-mode selection. */
    int stopped;                        /* Collector may inspect roots. */
    int resuming;                       /* Drain the previous parked generation before another collector. */
};


/* Reject cross-thread calls before inspecting owner-only mutable root state. */
static int owns(DVM_WorldParticipant *participant)
{
    return participant && dvm_native_context_current() == participant->context;
}


/* [GC:] Publish the current process at the same boundary as its stable registers. */
static void publish(DVM_WorldParticipant *participant)
{
    participant->process = dvm_native_context_process(participant->context);
}


/* [GC:] An active mutator parks with a complete instruction's roots visible. */
static int poll_mutator(void *user)
{
    DVM_WorldParticipant *participant = (DVM_WorldParticipant *)user;
    DVM_NativeWorld *world;
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    /* Only this owner changes active; the collector reads it under sync.
     * A no-request observation does not publish roots or admit collection. */
    if (!participant->active)
    {
        return 0;
    }
    if (!world->locked_poll && !dvm_world_request_load(&world->request))
    {
        participant->fast_polls++;
        return 1;
    }
    participant->locked_polls++;
    fvm_port_sync_lock(world->sync);
    if (!participant->active)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    publish(participant);
    if (world->collector)
    {
        participant->parked = 1;
        world->parked++;
        fvm_port_sync_signal_all(world->sync);
        while (world->collector)
        {
            fvm_port_sync_wait(world->sync);
        }
        world->parked--;
        participant->parked = 0;
        fvm_port_sync_signal_all(world->sync);
    }
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Keep the collecting up bound until resume; mutator execution is forbidden here. */
static int poll_collector(void *user)
{
    (void)user;
    return 0;
}


/* Allocate an owned empty coordination domain, never a VM heap. locked_poll
 * selects the mutex reference route when nonzero. Null reports native allocation
 * failure; no participant or collector may outlive the eventual destruction. */
DVM_NativeWorld *dvm_world_create_mode(int locked_poll)
{
    DVM_NativeWorld *world = (DVM_NativeWorld *)calloc(1, sizeof(*world));
    if (!world)
    {
        return 0;
    }
    world->sync = fvm_port_sync_create();
    if (!world->sync)
    {
        free(world);
        return 0;
    }
    world->locked_poll = locked_poll != 0;
    return world;
}


/* Create a FIFO exclusive-mutator domain. locked_poll selects the reference
 * instruction poll, not admission fairness. Returns an owned empty world or NULL.
 * Waiting owners keep published roots stable; native thread cancellation while
 * queued is unsupported. No allocation/GC or external execution mutex is added. */
DVM_NativeWorld *dvm_world_create_serial(int locked_poll)
{
    DVM_NativeWorld *world = dvm_world_create_mode(locked_poll);
    if (world)
    {
        world->serial = 1;
    }
    return world;
}


/* Create the default atomic-request domain; null reports native allocation failure. */
DVM_NativeWorld *dvm_world_create(void)
{
    return dvm_world_create_mode(0);
}


/* Caller quiesces outside users before destruction; registered roots forbid it. */
int dvm_world_destroy(DVM_NativeWorld *world)
{
    if (!world)
    {
        return 1;
    }
    fvm_port_sync_lock(world->sync);
    if (world->count || world->collector || world->resuming)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    fvm_port_sync_unlock(world->sync);
    fvm_port_sync_destroy(world->sync);
    free(world);
    return 1;
}


/* Install the poll hook before publishing a root set to a collector. */
DVM_WorldParticipant *dvm_world_join(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    DVM_WorldParticipant *participant;
    if (!world || !context || dvm_native_context_current() != context)
    {
        return 0;
    }
    participant = (DVM_WorldParticipant *)calloc(1, sizeof(*participant));
    if (!participant)
    {
        return 0;
    }
    participant->world = world;
    participant->context = context;
    participant->registers = dvm_native_context_registers(context);
    participant->register_size = dvm_native_context_register_size(context);
    publish(participant);
    fvm_port_sync_lock(world->sync);
    if (world->sealed || world->collector || !dvm_native_context_set_poll(context, poll_mutator, participant))
    {
        fvm_port_sync_unlock(world->sync);
        free(participant);
        return 0;
    }
    participant->next = world->participants;
    world->participants = participant;
    world->count++;
    fvm_port_sync_unlock(world->sync);
    return participant;
}


/* Remove only inactive roots on their owner; a collector never sees freed storage. */
int dvm_world_detach(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    DVM_WorldParticipant **link;
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (participant->active || world->collector ||
        !dvm_native_context_clear_poll(participant->context, poll_mutator, participant))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (link = &world->participants; *link != participant; link = &(*link)->next)
        ;
    *link = participant->next;
    world->count--;
    fvm_port_sync_unlock(world->sync);
    free(participant);
    return 1;
}


/* A new active interval cannot overlap an already requested root scan. */
int dvm_world_enter(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    size_t ticket = 0;
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (participant->active)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    if (world->serial)
    {
        ticket = world->next_ticket++;
        world->waiting++;
        if (world->active || world->collector || world->resuming || ticket != world->serving_ticket)
        {
            world->contentions++;
        }
    }
    while (world->collector || (world->serial &&
           (world->active || world->resuming || ticket != world->serving_ticket)))
    {
        fvm_port_sync_wait(world->sync);
    }
    if (world->serial)
    {
        world->waiting--;
        world->serving_ticket++;
        world->admissions++;
    }
    participant->active = 1;
    world->active++;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] Inactive means roots are stable even while the owner is blocked in I/O. */
int dvm_world_leave(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (!participant->active || participant->parked)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    publish(participant);
    participant->active = 0;
    world->active--;
    fvm_port_sync_signal_all(world->sync);
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Serialize collectors while allowing all mutators to run outside this mutex. */
int dvm_world_stop_begin(DVM_NativeWorld *world, DVM_NativeContext *collector)
{
    DVM_WorldParticipant *participant;
    if (!world || !collector || dvm_native_context_current() != collector)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    if (world->collector || world->parked || world->resuming)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (participant = world->participants; participant; participant = participant->next)
    {
        if (participant->context == collector)
        {
            fvm_port_sync_unlock(world->sync);
            return 0;
        }
    }
    if (!dvm_native_context_set_poll(collector, poll_collector, world))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    world->collector = collector;
    dvm_world_request_store(&world->request,1);
    fvm_port_sync_signal_all(world->sync);
    while (world->active != world->parked)
    {
        fvm_port_sync_wait(world->sync);
    }
    world->stopped = 1;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] No active participant can mutate its published roots during this visit. */
int dvm_world_visit(DVM_NativeWorld *world, DVM_WorldRoots visitor, void *user)
{
    DVM_WorldParticipant *participant;
    if (!world || !visitor)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    if (!world->stopped || !world->collector || world->collector != dvm_native_context_current())
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (participant = world->participants; participant; participant = participant->next)
    {
        visitor(participant->context, participant->registers, participant->register_size, participant->process, user);
    }
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Wait for the old parked generation to drain before allowing a new stop request. */
int dvm_world_stop_end(DVM_NativeWorld *world)
{
    if (!world)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    if (!world->stopped || !world->collector || world->collector != dvm_native_context_current())
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    world->stopped = 0;
    world->resuming = 1;
    world->collector = 0;
    dvm_world_request_store(&world->request,0);
    fvm_port_sync_signal_all(world->sync);
    while (world->parked)
    {
        fvm_port_sync_wait(world->sync);
    }
    if (!dvm_native_context_clear_poll(dvm_native_context_current(), poll_collector, world))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    world->resuming = 0;
    fvm_port_sync_signal_all(world->sync);
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Observe one coherent state without joining a collection rendezvous. */
int dvm_world_state(DVM_NativeWorld *world, DVM_WorldState *out)
{
    if (!world || !out)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    out->participants = world->count;
    out->active = world->active;
    out->parked = world->parked;
    out->stopping = world->collector != 0;
    out->stopped = world->stopped;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Copy owner-private poll counters. participant must belong to the calling
 * bound context; out parameters are caller-owned. No cross-owner counter reads,
 * heap access, blocking or collection admission occurs. Returns zero on misuse. */
int dvm_world_owner_polls(DVM_WorldParticipant *participant, size_t *fast, size_t *locked)
{
    if (!owns(participant) || !fast || !locked)
    {
        return 0;
    }
    *fast = participant->fast_polls;
    *locked = participant->locked_polls;
    return 1;
}


/* Copy serialized admission diagnostics under the world mutex. world/out are
 * borrowed live native objects; no reservation, heap access or state mutation.
 * Returns zero for invalid arguments, one for a coherent snapshot in either mode. */
int dvm_world_admission(DVM_NativeWorld *world, DVM_WorldAdmission *out)
{
    if (!world || !out)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    out->serial = world->serial;
    out->waiting = world->waiting;
    out->admissions = world->admissions;
    out->contentions = world->contentions;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Atomically refuse new registrations only for the last active serialized
 * owner. participant is borrowed/current; peers or invalid phase leave the
 * domain open. Existing owner can still finish and detach after success. */
int dvm_world_seal_last_owner(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (!world->serial || world->count != 1 || !participant->active || participant->parked)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    world->sealed = 1;
    fvm_port_sync_unlock(world->sync);
    return 1;
}
