/* Actual retained addw, two extended-up owners and FIFO exclusive heap turns. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "disvm/native_process.h"
#include "disvm/native_world.h"
#include "disvm/native_owner.h"
#include "native_wait.h"
#include "thread_port.h"
#include "channel_sync.h"

#define STRESS_TURNS 10000 /* Per-owner reentry iterations after the native-wait phase. */
#define SCANS 100 /* Root rendezvous while owner zero is outside the heap. */

char exBounds[] = "array bounds error"; /* Retained addw's rejected-span error identity. */


/* Fail the fixture, never turn a protocol/instruction fault into success.
 * message is borrowed; no return, managed allocation or VM unwind is simulated. */
void error(char *message)
{
    fprintf(stderr,"FAIL serialized native world: %s\n",message);
    exit(EXIT_FAILURE);
}


/* Validate a fixture predicate. message is borrowed; success has no side effects,
 * failure terminates all threads. Callers retain ownership of native resources. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        error((char *)message);
    }
}

PRODUCTION_ADDW


/* Control messages use their own native condition. Shared value/order are plain
 * instruction operands protected ONLY by the production world's admission gate. */
typedef struct Shared
{
    DVM_NativeWorld *world; /* Owned by main through worker joins. */
    FVM_PortSync *control; /* Protect phase flags, never the arithmetic operand. */
    unsigned long epoch;  /* Collector increments only under a world stop. */
    UWORD value;           /* Shared non-atomic addw source/destination. */
    unsigned int order[3]; /* First three admissions, written only during heap turns. */
    unsigned int used;     /* Initialized order entries, protected by the heap gate. */
    int joined;           /* Workers with registered, stable root sets. */
    int first_active;     /* Owner zero holds the initial heap turn. */
    int release_first;    /* Main observed owner one's queued ticket. */
    int inactive;         /* Owner zero waits in native code with saved roots. */
    int progressed;       /* Owner one executed at least 16384 adds during that wait. */
    int stress;           /* Main completed stopped-root checks; both may stress reentry. */
    int done;             /* Finished stress owners, still registered/inactive. */
    int retire;           /* Final collector observation finished; detach is allowed. */
} Shared;


/* Mutable instruction data belongs to this native owner inside admitted turns;
 * the stopped collector may read it only through its published root snapshot. */
typedef struct Worker
{
    Shared *shared;               /* Borrowed native control domain through join. */
    DVM_NativeContext *context;   /* Owned by main; bound/unbound by this worker. */
    DVM_HeapOwner *owner;         /* Borrowed context-owned per-thread lifecycle metadata. */
    DVM_HeapOwnerStats snapshot;   /* Copied by the owner before retirement. */
    unsigned long before;        /* Independent expected native interval start. */
    unsigned long expected_gc;   /* Independently accumulated stopped-epoch deltas. */
    unsigned long calls;         /* Independent expected native call count. */
    UWORD one;                   /* Constant native addw operand, initialized before join. */
    UWORD last;                  /* Last result published inside the admitted interval. */
    unsigned long iterations;    /* Actual retained addw calls inside admitted turns. */
    unsigned int identity;       /* Immutable owner zero or one. */
} Worker;


/* Wait for a protected phase flag. shared/flag are borrowed native storage and
 * stay live through all worker joins. No managed/root storage is changed here. */
static void wait_flag(Shared *shared, int *flag)
{
    fvm_port_sync_lock(shared->control);
    while (!*flag)
    {
        fvm_port_sync_wait(shared->control);
    }
    fvm_port_sync_unlock(shared->control);
}


/* Run count retained instructions during this owner's admitted heap interval.
 * worker/count are borrowed. Poll before each addw, then publish the local REG
 * mirror. No extra mutex or atomic counter protects the shared operand. */
static void add(Worker *worker, unsigned int count)
{
    unsigned int index;
    for (index = 0; index < count; index++)
    {
        check(dvm_native_context_poll(),"instruction poll");
        addw();
        worker->last = worker->shared->value;
        DVM_R.t = worker->last;
        worker->iterations++;
    }
}


/* Record one initial FIFO turn on shared native test data. worker is borrowed;
 * the caller already owns admission. Raises a fatal test failure on overflow. */
static void initial(Worker *worker)
{
    Shared *shared = worker->shared;
    if (worker->identity)
    {
        check(shared->used == 1 && shared->order[0] == 0,
            "FIFO peer admitted before first owner released");
    }
    check(shared->used < 3,"initial order capacity");
    shared->order[shared->used++] = worker->identity;
    add(worker,1);
}


/* Record an independent expected epoch before native suspension. worker is
 * borrowed/current/ACTIVE; only TLS metadata changes after production leave. */
static void suspend_owner(Worker *worker)
{
    worker->before = worker->shared->epoch;
    worker->calls++;
    check(dvm_native_owner_suspend(worker->owner,worker->before),"native suspend");
}


/* Resume before reading the shared collector epoch. worker remains borrowed;
 * independently validate the delta, pairing and refusal of unsampled transitions. */
static void resume_owner(Worker *worker)
{
    unsigned long epoch;
    check(dvm_native_owner_resume(worker->owner),"native resume");
    epoch = worker->shared->epoch;
    worker->expected_gc += epoch-worker->before;
    check(!dvm_native_owner_leave(worker->owner) &&
        !dvm_native_owner_suspend(worker->owner,epoch) &&
        !dvm_native_owner_seal(worker->owner),"require post-resume epoch sample");
    check(dvm_native_owner_progress(worker->owner,epoch),"sample active epoch");
    check(!dvm_native_owner_progress(worker->owner,epoch),"reject duplicate epoch sample");
}


/* OS worker entry; argument borrows one Worker. Register complete roots before
 * admission, demonstrate FIFO under contention, then simulate native waiting and
 * stress reentry. Published roots stay immutable while inactive. Detach/unbind
 * follow the final collector scan; main owns context destruction after join. */
static void execute(void *argument)
{
    Worker *worker = argument;
    Shared *shared = worker->shared;
    unsigned int index;
    int stress;
    check(dvm_native_context_bind(worker->context),"bind owner");
    check(dvm_native_context_set_process(worker->context,worker),"publish process");
    worker->one = 1;
    DVM_R.m = &worker->one;
    DVM_R.s = &shared->value;
    DVM_R.d = &shared->value;
    DVM_R.t = 0;
    worker->owner = dvm_native_owner_current();
    check(worker->owner != NULL && dvm_native_owner_attach(worker->owner,shared->world),"register owner roots");
    check(!dvm_native_owner_attach(worker->owner,shared->world),"reject double owner attach");
    check(!dvm_native_owner_resume(worker->owner),"reject resume before native suspend");
    check(!dvm_native_context_unbind(worker->context),"registered TLS cannot retire");
    fvm_port_sync_lock(shared->control);
    shared->joined++;
    fvm_port_sync_signal_all(shared->control);
    fvm_port_sync_unlock(shared->control);
    if (!worker->identity)
    {
        check(dvm_native_owner_enter(worker->owner),"first turn");
        check(!dvm_native_owner_enter(worker->owner),"reject nested turn without consuming a ticket");
        fvm_port_sync_lock(shared->control);
        shared->first_active = 1;
        fvm_port_sync_signal_all(shared->control);
        fvm_port_sync_unlock(shared->control);
        wait_flag(shared,&shared->release_first);
        check(!dvm_native_owner_seal(worker->owner),"refuse seal with a queued peer");
        check(!dvm_native_owner_try_share(worker->owner, shared->epoch),
            "shared transition must respect the older exclusive ticket");
        initial(worker);
        check(dvm_native_owner_leave(worker->owner),"release initial turn");
        check(dvm_native_owner_enter(worker->owner),"FIFO reentry");
        initial(worker);
        suspend_owner(worker);
        check(!dvm_native_owner_enter(worker->owner) && !dvm_native_owner_detach(worker->owner),
            "native interval cannot enter/detach as idle");
        fvm_port_sync_lock(shared->control);
        shared->inactive = 1;
        fvm_port_sync_signal_all(shared->control);
        fvm_port_sync_unlock(shared->control);
        wait_flag(shared,&shared->stress);
        resume_owner(worker);
        check(dvm_native_owner_leave(worker->owner),"end long native wait");
    }
    else
    {
        wait_flag(shared,&shared->first_active);
        check(dvm_native_owner_enter(worker->owner),"queued older ticket");
        initial(worker);
        check(dvm_native_owner_leave(worker->owner),"release older ticket");
        wait_flag(shared,&shared->inactive);
        index = 0;
        do
        {
            check(dvm_native_owner_enter(worker->owner),"native-wait peer admission");
            add(worker,64);
            suspend_owner(worker);
            resume_owner(worker);
            check(dvm_native_owner_leave(worker->owner),"native-wait peer release");
            index++;
            fvm_port_sync_lock(shared->control);
            if (index >= 256)
            {
                shared->progressed = 1;
                fvm_port_sync_signal_all(shared->control);
            }
            stress = shared->stress;
            fvm_port_sync_unlock(shared->control);
        } while (index < 256 || !stress);
    }
    for (index = 0; index < STRESS_TURNS; index++)
    {
        check(dvm_native_owner_enter(worker->owner),"stress enter");
        add(worker,1);
        check(dvm_native_owner_leave(worker->owner),"stress leave");
    }
    check(dvm_native_owner_enter(worker->owner),"final native sample enter");
    suspend_owner(worker);
    resume_owner(worker);
    check(dvm_native_owner_leave(worker->owner),"final sample leave");
    check(dvm_native_owner_statistics(worker->owner,&worker->snapshot) &&
        worker->snapshot.phase == DVM_HEAP_IDLE && !worker->snapshot.pending_progress &&
        worker->snapshot.releases == worker->calls && worker->snapshot.reentries == worker->calls &&
        worker->snapshot.gc_progress == worker->expected_gc,"owner epoch/call counters");
    fvm_port_sync_lock(shared->control);
    shared->done++;
    fvm_port_sync_signal_all(shared->control);
    fvm_port_sync_unlock(shared->control);
    wait_flag(shared,&shared->retire);
    check(dvm_native_owner_detach(worker->owner),"detach inactive roots");
    check(dvm_native_context_unbind(worker->context),"unbind owner");
}


/* One stopped-world observation, owned by main and borrowed by the visitor. */
typedef struct Scan
{
    unsigned int count; /* Exact live root registrations visited. */
    int native_wait;    /* Owner zero must retain its third arithmetic result. */
} Scan;


/* [GC:] Inspect actual REG/process roots only under world stop. All arguments
 * borrow storage alive through worker join. No allocation, locks, world reentry,
 * managed mutation or blocking. Fatal fixture failures cannot resume execution. */
static void roots(DVM_NativeContext *context, void *registers, size_t size,
    void *process, void *user)
{
    Worker *worker = process;
    REG *r = registers;
    Scan *scan = user;
    check(worker && context == worker->context && size == sizeof(REG),"exact owner roots");
    check(r->m == &worker->one && r->s == &worker->shared->value &&
        r->d == &worker->shared->value && (UWORD)r->t == worker->last,"complete stable REG");
    if (scan->native_wait && !worker->identity)
    {
        check(worker->iterations == 2 && worker->last == 3,"inactive owner roots unchanged");
    }
    scan->count++;
}


/* Stop all admitted work and inspect two stable root sets. shared/collector are
 * borrowed live native owners; native_wait selects the inactive-root invariant.
 * No timing assumption is used; begin waits for explicit instruction polls. */
static void scan_roots(Shared *shared, DVM_NativeContext *collector, int native_wait)
{
    DVM_WorldState state;
    Scan scan;
    scan.count = 0;
    scan.native_wait = native_wait;
    check(dvm_world_stop_begin(shared->world,collector),"collector begin");
    check(dvm_world_state(shared->world,&state) && state.active <= 1 &&
        state.active == state.parked && state.participants == 2,"exclusive stopped state");
    shared->epoch++;
    check(dvm_world_visit(shared->world,roots,&scan) && scan.count == 2,"visit both owners");
    check(dvm_world_stop_end(shared->world),"collector end");
}


/* Attempt attachment from a fresh bound OS context after sealing. argument is
 * a borrowed live world; no heap access/admission occurs. All native storage
 * retires on this thread. A reopened gate is a specific fatal fixture failure. */
static void late_join(void *argument)
{
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    check(context != NULL && dvm_native_context_bind(context),"late bind");
    check(!dvm_native_owner_attach(dvm_native_owner_current(),argument),
        "sealed owner registration reopened");
    check(dvm_native_context_unbind(context),"late unbind");
    check(dvm_native_context_destroy(context),"late context destroy");
}


/* Tutorial callback: read only while active. user is a borrowed native counter;
 * return its scalar value, checking the example's actual phase boundary. */
static unsigned long example_epoch(void *user)
{
    DVM_HeapOwnerStats state;
    check(dvm_native_owner_statistics(dvm_native_owner_current(),&state) &&
        state.phase == DVM_HEAP_ACTIVE,"example epoch sampled outside active ownership");
    return *(unsigned long *)user;
}


/* Tutorial callback: model a failing native operation and clock wrap. user
 * borrows a private native scalar, never managed/root storage. Return -7. */
static int example_failure(void *user)
{
    DVM_HeapOwnerStats state;
    check(dvm_native_owner_statistics(dvm_native_owner_current(),&state) &&
        state.phase == DVM_HEAP_NATIVE,"example operation did not release heap ownership");
    *(unsigned long *)user = 3;
    return -7;
}


/* Temporarily borrow ALL of this context's REG while keeping its native-call
 * metadata active. No safepoint or managed access occurs until restoration.
 * No args/result; this proves call state lives outside copied VM registers. */
static void register_isolation(void)
{
    REG saved;
    DVM_HeapOwnerStats before;
    DVM_HeapOwnerStats during;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_statistics(owner,&before) && before.phase == DVM_HEAP_ACTIVE,
        "register isolation starts active");
    memmove(&saved,&DVM_R,sizeof(saved));
    memset(&DVM_R,0x5a,sizeof(saved));
    check(dvm_native_owner_statistics(owner,&during) && during.phase == before.phase &&
        during.releases == before.releases && during.reentries == before.reentries &&
        during.gc_progress == before.gc_progress && during.pending_progress == before.pending_progress,
        "whole-REG borrowing corrupted native owner metadata");
    memmove(&DVM_R,&saved,sizeof(saved));
}


/* Main owns the world/control/contexts. argv optionally selects locked polling;
 * every run proves FIFO, real peer progress during native wait, exact arithmetic,
 * stable roots and full teardown. Return zero only after all joined resources. */
int main(int argc, char **argv)
{
    Shared shared;
    Worker workers[2];
    FVM_PortThread *threads[2];
    DVM_NativeContext *collector;
    DVM_WorldAdmission admission;
    unsigned int index;
    DVM_HeapOwnerStats private_copy;
    unsigned long example_clock = ULONG_MAX-5;
    int example_status = 99;
    memset(&shared,0,sizeof(shared));
    memset(workers,0,sizeof(workers));
    shared.world = dvm_world_create_serial(argc > 1 && strcmp(argv[1],"locked") == 0);
    shared.control = fvm_port_sync_create();
    collector = dvm_native_process_create(NULL);
    check(shared.world && shared.control && collector,"allocate domain");
    check(dvm_native_context_bind(collector),"bind collector");
    for (index = 0; index < 2; index++)
    {
        workers[index].shared = &shared;
        workers[index].identity = index;
        workers[index].context = dvm_native_process_create(NULL);
        check(workers[index].context != NULL,"allocate owner");
        threads[index] = fvm_port_thread_start(execute,&workers[index]);
        check(threads[index] != NULL,"start real OS owner");
    }
    /* No collection yet: owner zero deliberately holds its first turn until an
     * older peer ticket exists. This is a deterministic contention handshake. */
    do
    {
        check(dvm_world_admission(shared.world,&admission),"observe FIFO wait");
        if (!admission.waiting)
        {
            fvm_port_thread_sleep_us(50);
        }
    } while (!admission.waiting);
    fvm_port_sync_lock(shared.control);
    shared.release_first = 1;
    fvm_port_sync_signal_all(shared.control);
    fvm_port_sync_unlock(shared.control);
    wait_flag(&shared,&shared.progressed);
    check(!dvm_native_owner_enter(workers[0].owner),"reject foreign owner");
    check(!dvm_native_owner_statistics(workers[0].owner,&private_copy),"reject foreign private counter read");
    check(!dvm_world_destroy(shared.world),"reject live world destruction");
    for (index = 0; index < SCANS; index++)
    {
        scan_roots(&shared,collector,1);
    }
    fvm_port_sync_lock(shared.control);
    shared.stress = 1;
    fvm_port_sync_signal_all(shared.control);
    fvm_port_sync_unlock(shared.control);
    for (index = 0; index < SCANS; index++)
    {
        scan_roots(&shared,collector,0);
    }
    fvm_port_sync_lock(shared.control);
    while (shared.done != 2)
    {
        fvm_port_sync_wait(shared.control);
    }
    fvm_port_sync_unlock(shared.control);
    scan_roots(&shared,collector,0);
    check(workers[0].snapshot.releases == 2 && workers[1].snapshot.releases >= 257 &&
        workers[0].snapshot.gc_progress >= SCANS,"independent overlapping native waits");
    check(shared.used == 3 && shared.order[0] == 0 && shared.order[1] == 1 &&
        shared.order[2] == 0,"older waiter precedes releasing owner's reentry");
    check(shared.value == workers[0].iterations+workers[1].iterations,"no lost shared adds");
    check(dvm_world_admission(shared.world,&admission) && admission.serial &&
        !admission.waiting && admission.contentions &&
        admission.admissions >= 2*STRESS_TURNS+259,"FIFO counters fully drained");
    fvm_port_sync_lock(shared.control);
    shared.retire = 1;
    fvm_port_sync_signal_all(shared.control);
    fvm_port_sync_unlock(shared.control);
    for (index = 0; index < 2; index++)
    {
        fvm_port_thread_join(threads[index]);
        check(dvm_native_context_destroy(workers[index].context),"destroy joined owner");
    }
    check(dvm_native_owner_attach(dvm_native_owner_current(),shared.world),"attach last owner");
    check(dvm_native_owner_enter(dvm_native_owner_current()),"last owner enter");
    register_isolation();
    check(dvm_example_native_wait(dvm_native_owner_current(),example_epoch,&example_clock,
        example_failure,&example_clock,&example_status) && example_status == -7,
        "tutorial negative native status returned after reentry");
    check(dvm_native_owner_statistics(dvm_native_owner_current(),&private_copy) &&
        private_copy.gc_progress == 9 && private_copy.releases == 1 && private_copy.reentries == 1,
        "unsigned epoch wrap arithmetic");
    check(dvm_native_context_object_acquire(collector),"hold guard accounting");
    check(!dvm_native_owner_try_share(dvm_native_owner_current(), 3),
        "reject shared transition while a guard is held");
    check(dvm_native_context_object_release(collector),"release guard accounting");
    check(dvm_native_owner_try_share(dvm_native_owner_current(), 3),
        "convert uncontended exclusive turn to shared");
    check(dvm_world_shared_peak(shared.world) == 1,"record shared occupancy");
    check(dvm_native_context_object_acquire(collector),"hold shared guard accounting");
    check(!dvm_native_owner_resume_shared(dvm_native_owner_current()),
        "reject exclusive promotion while a guard is held");
    check(dvm_native_context_object_release(collector),"release shared guard accounting");
    check(dvm_native_owner_resume_shared(dvm_native_owner_current()) &&
        dvm_native_owner_progress(dvm_native_owner_current(), 3),
        "promote uncontended shared turn and acknowledge epoch");
    check(dvm_native_owner_statistics(dvm_native_owner_current(),&private_copy) &&
        private_copy.phase == DVM_HEAP_ACTIVE && private_copy.releases == 2 &&
        private_copy.reentries == 2 && !private_copy.pending_progress,
        "shared handoff preserves the native lease accounting");
    check(dvm_native_owner_seal(dvm_native_owner_current()),"seal last owner");
    {
        FVM_PortThread *late = fvm_port_thread_start(late_join,shared.world);
        check(late != NULL,"start late registration attempt");
        fvm_port_thread_join(late);
    }
    check(dvm_native_owner_leave(dvm_native_owner_current()),"last owner leave");
    check(dvm_native_owner_detach(dvm_native_owner_current()),"last owner detach");
    check(dvm_native_context_unbind(collector),"unbind collector");
    check(dvm_native_context_destroy(collector),"destroy collector");
    check(dvm_world_destroy(shared.world),"destroy empty domain");
    fvm_port_sync_destroy(shared.control);
    puts("PASS per-owner native leases: independent overlapping calls/epochs, FIFO, 201 root scans, sealed registration, complete joins");
    return 0;
}
