/* Native root rendezvous, with optional FIFO exclusive heap-mutator admission. */
#include "disvm/native_world.h"
#include "channel_sync.h"
#include "world_request.h"
#include <stdlib.h>
#include <limits.h>


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
    unsigned long admission_epoch; /* Epoch of the current interpreter/JIT heap interval. */
    unsigned long read_epoch;   /* Oldest live explicit borrow, or zero. */
    unsigned long read_depth;   /* Nested borrows held by this owner. */
    int active;                 /* Owner may execute mutator instructions. */
    int parked;                 /* Owner is currently waiting for resume. */
    int service;                /* Collector participant restricted to shared try-entry. */
    int shared;                 /* Active interval is restricted to audited shared work. */
};


/* [GC:] Mutex protects membership, root publication and stop predicates only.
 * It is never held while executing a normal mutator instruction. */
struct DVM_NativeWorld
{
    FVM_PortSync *sync;                 /* Guards state, never instruction bodies. */
    DVM_WorldParticipant *participants; /* Registered roots, including inactive ones. */
    DVM_NativeContext *collector;       /* Bound collecting owner, or null. */
    unsigned long reclaim_epoch;       /* Latest issued reader generation. */
    size_t count;                       /* Number of root sets. */
    size_t active;                      /* Mutators between enter and leave. */
    size_t parked;                      /* Active mutators stopped at poll. */
    size_t shared_peak;                 /* Highest simultaneous restricted mutator count. */
    size_t shared;                      /* Active restricted shared intervals. */
    long request;                       /* Atomic stop request, not root publication. */
    int sealed;                        /* Registration closed by the last active owner before shutdown. */
    int serial;                        /* Immutable exclusive-mutator mode; not a parallel heap barrier. */
    size_t next_ticket;                /* Next FIFO admission ticket under sync; unsigned wrap is intentional. */
    size_t serving_ticket;             /* Oldest queued admission; advances only on a successful enter. */
    size_t exclusive_waiting;          /* Queued exclusive owners. */
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


/* Register context roots in world and install their poll hook. Service roots
 * are for a collector that only tries shared admission. Return NULL on refusal. */
static DVM_WorldParticipant *join_world(DVM_NativeWorld *world, DVM_NativeContext *context, int service)
{
    DVM_WorldParticipant *participant;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!world || !context || dvm_native_context_current() != context)
    {
        return 0;
    }
    participant = (DVM_WorldParticipant *)calloc(1, sizeof(*participant));
    if (!participant)
    {
        return 0;
    }
    participant->service = service;
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


/* Register ordinary owner context roots in world. Return NULL on refusal. */
DVM_WorldParticipant *dvm_world_join(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    return join_world(world, context, 0);
}


/* Register a collector context for nonblocking shared work in world.
 * The caller detaches on its own context before the world is destroyed. */
DVM_WorldParticipant *dvm_world_join_service(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    return join_world(world, context, 1);
}


/* Remove only inactive roots on their owner; a collector never sees freed storage. */
int dvm_world_detach(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    DVM_WorldParticipant **link;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (participant->active || world->collector ||
        participant->read_depth ||
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
static int enter_world(DVM_WorldParticipant *participant, int shared, int promote, int try_only)
{
    DVM_NativeWorld *world;
    size_t ticket = 0;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!owns(participant))
    {
        return 0;
    }
    if (participant->service && (!shared || promote || !try_only))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (promote)
    {
        if (!participant->active || !participant->shared || participant->parked)
        {
            fvm_port_sync_unlock(world->sync);
            return 0;
        }
        if (world->active == 1 && !world->waiting && !world->collector && !world->resuming)
        {
            participant->shared = 0;
            world->shared--;
            world->admissions++;
            fvm_port_sync_unlock(world->sync);
            return 1;
        }
        /* Publish before becoming inactive, and queue before unlocking.
         * Earlier tickets and an outstanding collector keep precedence. */
        publish(participant);
        participant->active = 0;
        participant->shared = 0;
        world->active--;
        world->shared--;
        fvm_port_sync_signal_all(world->sync);
    }
    else if (participant->active || (shared && (!world->serial || world->sealed)))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    if (try_only && (world->collector || world->resuming || world->waiting ||
        world->active != world->shared))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    if (world->serial)
    {
        ticket = world->next_ticket++;
        world->waiting++;
        world->exclusive_waiting += !shared;
        if ((world->active && (!shared || world->active != world->shared)) ||
            world->collector || world->resuming || ticket != world->serving_ticket)
        {
            world->contentions++;
        }
    }
    while (world->collector || (world->serial &&
           ((world->active && (!shared || world->active != world->shared)) ||
            world->resuming || ticket != world->serving_ticket)))
    {
        fvm_port_sync_wait(world->sync);
    }
    if (world->serial)
    {
        world->waiting--;
        world->exclusive_waiting -= !shared;
        world->serving_ticket++;
        world->admissions++;
    }
    participant->admission_epoch = world->reclaim_epoch + 1;
    participant->active = 1;
    participant->shared = shared;
    world->active++;
    if (shared)
    {
        world->shared++;
        if (world->shared > world->shared_peak)
        {
            world->shared_peak = world->shared;
        }
        /* A following shared ticket can enter this same group. */
        fvm_port_sync_signal_all(world->sync);
    }
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Announce a borrow before reading a published pointer. Nesting keeps the
 * oldest epoch until the last matching end. No heap admission is granted. */
int dvm_world_read_begin(DVM_WorldParticipant *participant)
{
    if (!owns(participant))
    {
        return 0;
    }
    DVM_NativeWorld *world = participant->world;
    fvm_port_sync_lock(world->sync);
    int ok = participant->read_depth != ULONG_MAX;
    if (ok)
    {
        if (!participant->read_depth)
        {
            participant->read_epoch = world->reclaim_epoch + 1;
        }
        participant->read_depth++;
    }
    fvm_port_sync_unlock(world->sync);
    return ok;
}


/* End one owner-local borrow after the final pointer access. */
int dvm_world_read_end(DVM_WorldParticipant *participant)
{
    if (!owns(participant))
    {
        return 0;
    }
    DVM_NativeWorld *world = participant->world;
    fvm_port_sync_lock(world->sync);
    int ok = participant->read_depth != 0;
    if (ok && --participant->read_depth == 0)
    {
        participant->read_epoch = 0;
    }
    fvm_port_sync_unlock(world->sync);
    return ok;
}


/* Exclusive callers use this before transferring a closed graph. */
int dvm_world_has_borrowers(DVM_NativeWorld *world)
{
    int borrowed = 0;
    fvm_port_sync_lock(world->sync);
    for (DVM_WorldParticipant *reader = world->participants;
         reader && !borrowed; reader = reader->next)
    {
        borrowed = reader->read_depth != 0;
    }
    fvm_port_sync_unlock(world->sync);
    return borrowed;
}


/* Capture a retirement generation after withdrawing the pointer from new
 * readers. A zero result at counter exhaustion disables concurrent reclaim. */
unsigned long dvm_world_retire_epoch(DVM_NativeWorld *world)
{
    if (!world)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    unsigned long epoch = 0;
    if (world->reclaim_epoch < ULONG_MAX - 1)
    {
        epoch = ++world->reclaim_epoch;
    }
    fvm_port_sync_unlock(world->sync);
    return epoch;
}


/* Observe grace-period completion without parking any participant. The
 * retained stopped collector is conservatively treated as a live reader. */
int dvm_world_epoch_elapsed(DVM_NativeWorld *world, unsigned long epoch)
{
    if (!world || !epoch)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    int elapsed = epoch <= world->reclaim_epoch && !world->collector;
    for (DVM_WorldParticipant *reader = world->participants; elapsed && reader; reader = reader->next)
    {
        elapsed = (!reader->active || reader->admission_epoch > epoch) &&
            (!reader->read_depth || reader->read_epoch > epoch);
    }
    fvm_port_sync_unlock(world->sync);
    return elapsed;
}


/* Enter the ordinary exclusive interval in a serialized world. */
int dvm_world_enter(DVM_WorldParticipant *participant)
{
    return enter_world(participant, 0, 0, 0);
}


/* Enter an audited shared interval without bypassing earlier FIFO tickets. */
int dvm_world_enter_shared(DVM_WorldParticipant *participant)
{
    return enter_world(participant, 1, 0, 0);
}


/* Attempt one shared turn without waiting behind a collector or older ticket. */
int dvm_world_try_enter_shared(DVM_WorldParticipant *participant)
{
    return enter_world(participant, 1, 0, 1);
}


/* Keep the current turn only when it cannot jump a queued owner or GC. */
int dvm_world_try_share(DVM_WorldParticipant *participant)
{
    if (dvm_native_context_object_count(dvm_native_context_current()) || !owns(participant))
    {
        return 0;
    }
    DVM_NativeWorld *world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (!world->serial || world->sealed || world->waiting || world->collector ||
        world->resuming || !participant->active || participant->shared || participant->parked)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    publish(participant);
    participant->shared = 1;
    world->shared++;
    if (world->shared > world->shared_peak)
    {
        world->shared_peak = world->shared;
    }
    fvm_port_sync_unlock(world->sync);
    return 1;
}


int dvm_world_promote_shared(DVM_WorldParticipant *participant)
{
    return enter_world(participant, 0, 1, 0);
}


/* [GC:] Inactive means roots are stable even while the owner is blocked in I/O. */
int dvm_world_leave(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
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
    if (participant->shared)
    {
        participant->shared = 0;
        world->shared--;
    }
    world->active--;
    fvm_port_sync_signal_all(world->sync);
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Serialize collectors while allowing all mutators to run outside this mutex. */
static int stop_begin(DVM_NativeWorld *world, DVM_NativeContext *collector, int quiescent)
{
    DVM_WorldParticipant *participant;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!world || !collector || dvm_native_context_current() != collector)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    if (world->collector || world->parked || world->resuming || (quiescent && world->active))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (participant = world->participants; participant; participant = participant->next)
    {
        if (participant->context == collector || (quiescent && participant->read_depth))
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
    if (!quiescent)
    {
        dvm_world_request_store(&world->request,1);
        fvm_port_sync_signal_all(world->sync);
        while (world->active != world->parked)
        {
            fvm_port_sync_wait(world->sync);
        }
    }
    world->stopped = 1;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* Request the retained stop rendezvous. */
int dvm_world_stop_begin(DVM_NativeWorld *world, DVM_NativeContext *collector)
{
    return stop_begin(world, collector, 0);
}


/* Claim an already quiescent heap without asking active owners to park. */
int dvm_world_try_quiescent(DVM_NativeWorld *world, DVM_NativeContext *collector)
{
    return stop_begin(world, collector, 1);
}


/* Do not reserve admission. The caller must retry the full quiescence check. */
void dvm_world_wait_quiescent_retry(DVM_NativeWorld *world)
{
    fvm_port_sync_lock(world->sync);
    int busy = world->active || world->collector || world->parked || world->resuming;
    for (DVM_WorldParticipant *participant = world->participants;
         !busy && participant; participant = participant->next)
    {
        busy = participant->read_depth != 0;
    }
    if (busy)
    {
        fvm_port_sync_wait_ms(world->sync, 1);
    }
    fvm_port_sync_unlock(world->sync);
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
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
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


/* Observe exclusive waiters in the borrowed live world under its mutex. */
size_t dvm_world_exclusive_waiting(DVM_NativeWorld *world)
{
    if (!world)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    size_t count = world->exclusive_waiting;
    fvm_port_sync_unlock(world->sync);
    return count;
}


/* Observe the highest simultaneous SHARED occupancy under the world mutex. */
size_t dvm_world_shared_peak(DVM_NativeWorld *world)
{
    size_t peak;
    if (!world)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    peak = world->shared_peak;
    fvm_port_sync_unlock(world->sync);
    return peak;
}


/* Atomically refuse new registrations only for the last active serialized
 * owner. participant is borrowed/current; peers or invalid phase leave the
 * domain open. Existing owner can still finish and detach after success. */
int dvm_world_seal_last_owner(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (!world->serial || world->active != 1 || !participant->active || participant->parked || participant->shared)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (DVM_WorldParticipant *other = world->participants; other; other = other->next)
    {
        if (other != participant && (!other->service || other->read_depth))
        {
            fvm_port_sync_unlock(world->sync);
            return 0;
        }
    }
    world->sealed = 1;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] Verify exact collecting owner under the world mutex, without traversing
 * roots. world borrowed/live; false for unbound/guarded/foreign or unstopped use.
 * A true result stays valid until this same collector ends its stop interval. */
int dvm_world_collector_owns(DVM_NativeWorld *world)
{
    DVM_NativeContext *context = dvm_native_context_current();
    int result;
    if (!world || !context || dvm_native_context_object_count(context))
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    result = world->stopped && world->collector == context;
    fvm_port_sync_unlock(world->sync);
    return result;
}
