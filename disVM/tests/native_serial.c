/* Actual retained addw, two extended-up owners and FIFO exclusive heap turns. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "disvm/native_process.h"
#include "disvm/native_world.h"
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
    DVM_WorldParticipant *roots; /* Registration remains live until main permits retirement. */
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
    worker->roots = dvm_world_join(shared->world,worker->context);
    check(worker->roots != NULL,"register roots");
    fvm_port_sync_lock(shared->control);
    shared->joined++;
    fvm_port_sync_signal_all(shared->control);
    fvm_port_sync_unlock(shared->control);
    if (!worker->identity)
    {
        check(dvm_world_enter(worker->roots),"first turn");
        check(dvm_world_read_begin(worker->roots) && dvm_world_read_begin(worker->roots),
            "announce nested raw borrows");
        check(!dvm_world_enter(worker->roots),"reject nested turn without consuming a ticket");
        fvm_port_sync_lock(shared->control);
        shared->first_active = 1;
        fvm_port_sync_signal_all(shared->control);
        fvm_port_sync_unlock(shared->control);
        wait_flag(shared,&shared->release_first);
        initial(worker);
        check(dvm_world_leave(worker->roots),"release initial turn");
        check(dvm_world_enter(worker->roots),"FIFO reentry");
        initial(worker);
        check(dvm_world_leave(worker->roots),"publish native-wait roots");
        fvm_port_sync_lock(shared->control);
        shared->inactive = 1;
        fvm_port_sync_signal_all(shared->control);
        fvm_port_sync_unlock(shared->control);
        wait_flag(shared,&shared->stress);
        check(dvm_world_read_end(worker->roots) && dvm_world_read_end(worker->roots),
            "finish both raw borrows");
    }
    else
    {
        wait_flag(shared,&shared->first_active);
        check(!dvm_world_try_enter_shared(worker->roots),
            "collector-style shared attempt must refuse an active exclusive owner");
        check(dvm_world_enter(worker->roots),"queued older ticket");
        initial(worker);
        check(dvm_world_leave(worker->roots),"release older ticket");
        wait_flag(shared,&shared->inactive);
        index = 0;
        do
        {
            check(dvm_world_enter(worker->roots),"native-wait peer admission");
            add(worker,64);
            check(dvm_world_leave(worker->roots),"native-wait peer release");
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
        check(dvm_world_enter(worker->roots),"stress enter");
        add(worker,1);
        check(dvm_world_leave(worker->roots),"stress leave");
    }
    fvm_port_sync_lock(shared->control);
    shared->done++;
    fvm_port_sync_signal_all(shared->control);
    fvm_port_sync_unlock(shared->control);
    wait_flag(shared,&shared->retire);
    check(dvm_world_detach(worker->roots),"detach inactive roots");
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
    check(dvm_world_visit(shared->world,roots,&scan) && scan.count == 2,"visit both owners");
    check(dvm_world_stop_end(shared->world),"collector end");
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
    DVM_WorldState busy_state;
    check(!dvm_world_try_quiescent(shared.world, collector) &&
        dvm_world_state(shared.world, &busy_state) && !busy_state.stopping &&
        busy_state.active == 1 && !busy_state.parked,
        "Fast idle claim must not stop an active guest");
    unsigned long retired_epoch = dvm_world_retire_epoch(shared.world);
    check(retired_epoch && !dvm_world_epoch_elapsed(shared.world, retired_epoch),
        "old reader defers reclamation");
    fvm_port_sync_lock(shared.control);
    shared.release_first = 1;
    fvm_port_sync_signal_all(shared.control);
    fvm_port_sync_unlock(shared.control);
    wait_flag(&shared,&shared.progressed);
    check(!dvm_world_epoch_elapsed(shared.world, retired_epoch),
        "inactive old reader still pins retirement while peer progresses");
    check(!dvm_world_enter(workers[0].roots),"reject foreign owner");
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
    check(dvm_world_try_quiescent(shared.world, collector) &&
        dvm_world_stop_end(shared.world), "Fast claims idle heap without a rendezvous");
    check(dvm_world_epoch_elapsed(shared.world, retired_epoch),
        "retirement becomes eligible after old borrows end");
    check(dvm_native_context_unbind(collector),"unbind collector");
    check(dvm_native_context_destroy(collector),"destroy collector");
    check(dvm_world_destroy(shared.world),"destroy empty domain");
    fvm_port_sync_destroy(shared.control);
    puts("PASS FIFO heap admission: two native REG owners, shared addw, native-wait progress, 201 root scans, complete joins");
    return 0;
}
