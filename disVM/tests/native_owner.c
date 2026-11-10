/* Copyright (c) 2026 bhgv
 * EN: Two permanent native owners, guarded shared arithmetic and GC root scans.
 * RU: Два постоянных нативных владельца, защищённая арифметика и корни GC.
 */
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
    fprintf(stderr,"FAIL concurrent native owners: %s\n",message);
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


/* EN: Control phases and one shared operand have independent native mutexes.
 * RU: Фазы проверки и общий операнд защищены отдельными нативными мьютексами. */
typedef struct Shared
{
    DVM_NativeWorld *world; /* Owned by main through worker joins. */
    FVM_PortSync *control; /* Protect phase flags. */
    FVM_PortSync *operand; /* Protect the shared arithmetic operand. */
    unsigned long epoch;  /* Collector increments only under a world stop. */
    UWORD value;           /* Shared non-atomic addw source/destination. */
    unsigned int order[3]; /* First three admissions, written only during heap turns. */
    unsigned int used;     /* Initialized order entries, protected by control. */
    int joined;           /* Workers with registered, stable root sets. */
    int first_active;     /* Owner zero holds the initial heap turn. */
    int peer_active;      /* Both native owners hold admission. */
    int overlap_verified; /* Owner zero checked simultaneous participation. */
    int peer_finished;    /* Owner one finished its first arithmetic operation. */
    int release_first;    /* Main observed both admitted owners. */
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
 * mirror. The operand mutex spans one add and ends before the next poll.
 * RU: Проверить GC перед addw и защитить общий операнд отдельным мьютексом.
 * worker заимствован. Блокировка снимается до следующего опроса GC. */
static void add(Worker *worker, unsigned int count)
{
    unsigned int index;
    for (index = 0; index < count; index++)
    {
        check(dvm_native_context_poll(),"instruction poll");
        fvm_port_sync_lock(worker->shared->operand);
        addw();
        worker->last = worker->shared->value;
        fvm_port_sync_unlock(worker->shared->operand);
        DVM_R.t = worker->last;
        worker->iterations++;
    }
}


/* EN: Record an initial operation under the phase mutex, then add one.
 * worker is borrowed. Initial phases finish before the first GC stop.
 * RU: Записать начальную операцию под мьютексом фаз и прибавить единицу.
 * worker заимствован. Начальные фазы завершаются до первого GC. */
static void initial(Worker *worker)
{
    Shared *shared = worker->shared;
    fvm_port_sync_lock(shared->control);
    check(shared->used < 3,"initial order capacity");
    shared->order[shared->used++] = worker->identity;
    fvm_port_sync_unlock(shared->control);
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
 * admission, demonstrate concurrent entry, then simulate native waiting and
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
        check(!dvm_native_owner_enter(worker->owner),"reject nested active entry");
        initial(worker);
        fvm_port_sync_lock(shared->control);
        shared->first_active = 1;
        fvm_port_sync_signal_all(shared->control);
        fvm_port_sync_unlock(shared->control);
        wait_flag(shared,&shared->peer_active);
        check(!dvm_native_owner_seal(worker->owner), "refuse seal with another active owner");
        fvm_port_sync_lock(shared->control);
        shared->overlap_verified = 1;
        fvm_port_sync_signal_all(shared->control);
        fvm_port_sync_unlock(shared->control);
        wait_flag(shared,&shared->release_first);
        wait_flag(shared,&shared->peer_finished);
        check(dvm_native_owner_leave(worker->owner),"release initial turn");
        check(dvm_native_owner_enter(worker->owner),"same-owner reentry");
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
        check(dvm_native_owner_enter(worker->owner),"concurrent peer admission");
        initial(worker);
        fvm_port_sync_lock(shared->control);
        shared->peer_active = 1;
        fvm_port_sync_signal_all(shared->control);
        fvm_port_sync_unlock(shared->control);
        wait_flag(shared,&shared->release_first);
        check(dvm_native_owner_leave(worker->owner),"release concurrent peer");
        fvm_port_sync_lock(shared->control);
        shared->peer_finished = 1;
        fvm_port_sync_signal_all(shared->control);
        fvm_port_sync_unlock(shared->control);
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
    check(dvm_world_state(shared->world,&state) && state.active <= 2 &&
        state.active == state.parked && state.participants == 2,"stopped collector state");
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


/* Keep a service registered across sealing. Shared is borrowed through join.
 * The worker owns its bound context until it has detached from the world. */
static void idle_service(void *argument)
{
    Shared *shared = argument;
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    check(context && dvm_native_context_bind(context), "bind GC service");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_attach_service(owner, shared->world), "register GC service");
    check(!dvm_native_owner_enter(owner), "GC service cannot enter as an ACTIVE mutator");
    fvm_port_sync_lock(shared->control);
    shared->joined = 1;
    fvm_port_sync_signal_all(shared->control);
    fvm_port_sync_unlock(shared->control);
    wait_flag(shared, &shared->retire);
    check(!dvm_native_owner_try_enter_idle_shared(owner), "sealed world refuses GC service turn");
    check(dvm_native_owner_detach(owner), "detach idle GC service after sealing");
    check(dvm_native_context_unbind(context), "unbind GC service");
    check(dvm_native_context_destroy(context), "destroy GC service");
}


PRODUCTION_DRIVER
