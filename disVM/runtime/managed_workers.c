/* Fixed/elastic OS executor pool; roots and heap are serialized by NativeWorld. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "managed_workers.h"
#include "runtime_gc.h"
#include "runqueue.h"
#include <limits.h>
#include <pthread.h>
#include "channel_sync.h"
#include "thread_port.h"
#include "thread_registrar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* One helper slot has one writer while alive; the controller reads after join.
 * Native identity is published/read only under the shared heap gate. */
typedef struct ManagedWorker
{
    struct ManagedWorker *wake_next; /* Registered under service.sync until thread exit. */
    DVM_OwnerMessage *staged_reply; /* Owner-only reply removed before heap admission. */
    int reply_in_transit;       /* Published to the controller until the owner takes the reply. */
    FVM_PortSync *wake;         /* Dedicated readiness condition and generation lock. */
    int wake_stopped;           /* Stop/failure latch under wake. */
    struct ManagedWorker *next; /* Native registry link, changed only under heap exclusion. */
    unsigned long id;           /* Stable lifetime ID; never reused while the service exists. */
    Prog *guest;                /* Borrowed until retirement on its own thread. */
    int guest_pid;              /* Dedicated guest identity, or zero for pool execution. */
    int guest_retired;          /* Set by this guest thread after managed retirement. */
    int done;                   /* Published under service.sync after TLS destruction. */
    DVM_ThreadSlot *thread;    /* Registrar handle, joined before this slot is reset. */
    uintptr identity;          /* Live helper context identity; zero before/after attachment. */
    unsigned long turns;       /* Successful bytecode turns executed by this helper. */
    unsigned long handoffs;    /* Completed rootless scheduler releases, distinct from native I/O. */
    DVM_HeapOwnerStats owner;   /* Copied counters before the helper detaches. */
} ManagedWorker;


/* Native lifecycle only. Never hold sync while entering/waiting on the heap.
 * All slots and immutable callback data are published before thread creation. */
typedef struct ManagedWorkers
{
    DVM_ThreadRegistrar *registrar; /* Owns OS handles, independent of managed admission. */
    FVM_PortSync *sync;             /* Protect stop/failure/ready publication. */
    DVM_NativeWorld *world;         /* Borrowed shared heap domain, immutable through join. */
    int (*turn)(void);             /* Shared scheduler callback under heap exclusion. */
    int stopping;                 /* Cooperative stop request, under sync. */
    int failed;                   /* Latched attach/dispatch/retirement/start error. */
    int ready;                    /* Helpers that acquired their first heap turn, under sync. */
    int awaiting_start;           /* Controller-only admission rendezvous flag. */
    int automatic;                /* Elastic growth/idle retirement selected explicitly. */
    int diagnostics;              /* Emit lifecycle evidence only for --stats. */
    unsigned long limit;          /* Optional total live executor limit; zero is uncapped. */
    unsigned long live;           /* Live helpers, under heap exclusion. */
    unsigned long peak;           /* Maximum total live executors including controller. */
    unsigned long created;        /* Successfully created helper lifetime IDs. */
    unsigned long idle_reported;  /* Creation generation last reported fully idle/reaped. */
    unsigned long generation;     /* Runnable-work publication under sync; prevents lost wakeups. */
    unsigned long returning;      /* Native completions queued for heap reentry, under sync. */
    unsigned long reaped;         /* Helpers joined during rootless scheduling. */
    unsigned long retired_turns;  /* Counters accumulated from reaped helper slots. */
    unsigned long retired_releases; /* Audited releases accumulated after join. */
    unsigned long retired_reentries; /* Audited reentries accumulated after join. */
    void (*cancel_waits)(void);    /* Native wakeup hook for a fatal worker failure. */
    int count;                    /* Requested executor count, including the controller. */
    unsigned long controller;     /* Successful controller turns. */
    unsigned long files;          /* Descriptor operations admitted through native per-slot FIFO. */
    unsigned long ui_requests;    /* Helper UI role admissions, under heap exclusion. */
    ManagedWorker *wake_slots;      /* Dedicated conditions protected by service.sync. */
    ManagedWorker *helpers;       /* Individually allocated stable slots; never realloc live storage. */
} ManagedWorkers;

static __thread ManagedWorker *current_guest; /* Dedicated slot bound through native TLS teardown. */
static ManagedWorkers service; /* One process-wide runtime; lifetime ends after join. */


/* Copy the native stop/failure predicate without acquiring any heap state. */
static int stopping(void)
{
    int result;
    fvm_port_sync_lock(service.sync);
    result = service.stopping || service.failed;
    fvm_port_sync_unlock(service.sync);
    return result;
}


/* Wake every dedicated guest only for stop or fatal failure.
 * Caller owns service.sync, which pins the registered conditions. */
static void stop_dedicated_waits(void)
{
    ManagedWorker *worker;
    for (worker = service.wake_slots; worker; worker = worker->wake_next)
    {
        fvm_port_sync_lock(worker->wake);
        worker->wake_stopped = 1;
        fvm_port_sync_signal_all(worker->wake);
        fvm_port_sync_unlock(worker->wake);
    }
}


/* Remove a dedicated condition from the stop registry before publishing done. */
static void unregister_dedicated_wait(ManagedWorker *worker)
{
    ManagedWorker **slot = &service.wake_slots;
    while (*slot && *slot != worker)
    {
        slot = &(*slot)->wake_next;
    }
    if (*slot)
    {
        *slot = worker->wake_next;
    }
}


/* Latch a native failure. message is a stable diagnostic, never a managed value.
 * Callers retain their heap phase and must still retire the context safely. */
static void fail(const char *message)
{
    fprintf(stderr,"disvm managed worker: %s\n",message);
    fvm_port_sync_lock(service.sync);
    service.failed = 1;
    stop_dedicated_waits();
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
    service.cancel_waits();
}


/* Rootless scheduler suspension differs from an audited native call: the turn
 * token and both current-process slots must be empty and REG already saved.
 * Return one only after leaving the heap; no C-held managed root is admitted. */
static int leave_empty(DVM_NativeWorldSite site)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!context || !up || up->env != &up->defenv || up->prog ||
        dvm_native_context_process(context) || dvm_native_context_turn(context))
    {
        return 0;
    }
    /* NATIVEWORLD: release a rootless exclusive turn before sleeping or joining. */
    if (!dvm_native_owner_leave(dvm_native_owner_current()))
    {
        return 0;
    }
    dvm_native_execution()->world_last_exit = site;
    return 1;
}


/* EN: Read only published readiness and FIFO state while outside NativeWorld.
 * RU: Вне NativeWorld читать только опубликованную готовность и состояние FIFO. */
static int dedicated_work_pending(ManagedWorker *worker)
{
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)worker->guest;
    return __atomic_load_n(&resident->queued, __ATOMIC_ACQUIRE) ||
        dvm_messages_pending(resident->messages, 0, 0) ||
        dvm_messages_pending(resident->messages, 1, 0);
}


/* EN: Wait for the dedicated resident's first or next work notification.
 * RU: Дождаться первой или следующей работы закреплённого процесса. */
static void wait_dedicated_work(ManagedWorker *worker)
{
    fvm_port_sync_lock(worker->wake);
    while (!worker->wake_stopped && !dedicated_work_pending(worker))
    {
        fvm_port_sync_wait(worker->wake);
    }
    fvm_port_sync_unlock(worker->wake);
}


/* EN: Take one native reply after waking, before entering NativeWorld.
 * RU: Извлечь один нативный ответ после пробуждения, до входа в NativeWorld. */
static void stage_dedicated_reply(ManagedWorker *worker)
{
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)worker->guest;
    __atomic_store_n(&worker->reply_in_transit, 1, __ATOMIC_RELEASE);
    worker->staged_reply = dvm_messages_take(resident->messages, 1, 0);
    if (!worker->staged_reply)
    {
        __atomic_store_n(&worker->reply_in_transit, 0, __ATOMIC_RELEASE);
    }
}


/* Release an empty scheduler turn, join already terminated helpers without the
 * heap, then reclaim their native slots after reentry. Any rootless executor
 * can reap, including when the controller is blocked in native I/O. */
static int yield_empty(int idle, int rendezvous, int (*pending)(void))
{
    ManagedWorker **slot = &service.helpers;
    ManagedWorker *finished = NULL;
    ManagedWorker *worker;
    unsigned long generation;
    unsigned long idle_created = 0;
    unsigned long idle_reaped = 0;
    int wait_for_work;
    fvm_port_sync_lock(service.sync);
    generation = service.generation;
    fvm_port_sync_unlock(service.sync);
    if (pending && pending())
    {
        return !stopping();
    }
    wait_for_work = idle && !rendezvous &&
        (service.count == 1 || (service.automatic && service.live == 1));
    while (*slot)
    {
        int done;
        worker = *slot;
        fvm_port_sync_lock(service.sync);
        done = worker->done;
        fvm_port_sync_unlock(service.sync);
        if (done && !worker->guest)
        {
            *slot = worker->next;
            worker->next = finished;
            finished = worker;
        }
        else
        {
            slot = &worker->next;
        }
    }
    if (service.automatic && idle && service.live == 1 && !finished && service.reaped && service.helpers && !service.helpers->next &&
        service.idle_reported != service.created)
    {
        service.idle_reported = service.created;
        idle_created = service.created;
        idle_reaped = service.reaped;
    }
    if (!leave_empty(DVM_NW_SITE_IDLE))
    {
        /* The caller still owns the heap. Keep every unjoined slot visible
         * to shutdown even when rootless admission is refused. */
        while (finished)
        {
            worker = finished;
            finished = worker->next;
            worker->next = service.helpers;
            service.helpers = worker;
        }
        return 0;
    }
    for (worker = finished; worker; worker = worker->next)
    {
        if (!dvm_thread_registrar_join(service.registrar, worker->thread))
        {
            fprintf(stderr, "disvm: cannot join registered worker\n");
            abort();
        }
    }
    if (rendezvous && service.awaiting_start)
    {
        fvm_port_sync_lock(service.sync);
        while (service.ready < service.count - 1 && !service.stopping && !service.failed)
        {
            fvm_port_sync_wait(service.sync);
        }
        service.awaiting_start = 0;
        fvm_port_sync_unlock(service.sync);
    }
    if (idle_created && service.diagnostics)
    {
        fprintf(stderr, "NATIVEPOOL_IDLE executors=2 created=%lu reaped=%lu\n", idle_created, idle_reaped);
    }
    if (current_guest && idle && !finished)
    {
        wait_dedicated_work(current_guest);
        if (stopping())
        {
            return 0;
        }
        stage_dedicated_reply(current_guest);
    }
    else if (wait_for_work && !finished)
    {
        fvm_port_sync_lock(service.sync);
        while (service.generation == generation && !service.stopping && !service.failed)
        {
            fvm_port_sync_wait(service.sync);
        }
        fvm_port_sync_unlock(service.sync);
    }
    else if (idle)
    {
        fvm_port_thread_sleep_us(1000);
    }
    /* NATIVEWORLD: return to exclusive admission after the rootless wait. */
    DVM_NativeExecution *execution = dvm_native_execution();
    execution->world_entry_site = DVM_NW_SITE_IDLE;
    execution->world_entry_pending = 1;
    if (!dvm_native_owner_enter(dvm_native_owner_current()))
    {
        /* Returning to a managed error handler requires ACTIVE ownership. */
        fprintf(stderr, "disvm: cannot reenter after empty scheduler yield\n");
        exit(1);
    }
    execution->world_entry_pending = 0;
    while (finished)
    {
        worker = finished;
        finished = worker->next;
        service.reaped++;
        service.retired_turns += worker->turns;
        service.retired_releases += worker->owner.releases;
        service.retired_reentries += worker->owner.reentries;
        if (service.diagnostics)
        {
            fprintf(stderr, "NATIVEPOOL_REAP id=%lu joined=1 turns=%lu\n", worker->id, worker->turns);
        }
        if (worker->wake)
        {
            fvm_port_sync_destroy(worker->wake);
        }
        free(worker);
    }
    return !stopping();
}


/* Run real bytecode turns with independent up/REG/error stacks on this OS thread.
 * argument borrows its stable native helper slot until join. All managed access
 * is inside world admission; no slot contains a managed root.
 * No UI event pump runs here. Managed faults return through the same turn logic. */
static void execute(void *argument)
{
    int cancellation = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    DVM_NativeContext *context = cancellation ? NULL : dvm_native_process_create(NULL);
    ManagedWorker *worker = argument;
    int attached = 0;
    unsigned int idle_turns = 0;
    if (!context || !dvm_native_context_bind(context))
    {
        fail(cancellation ? "cannot disable executor thread cancellation" : "cannot bind executor");
        if (context)
        {
            dvm_native_context_destroy(context);
        }
        fvm_port_sync_lock(service.sync);
        unregister_dedicated_wait(worker);
        worker->done = 1;
        service.generation++;
        fvm_port_sync_signal_all(service.sync);
        fvm_port_sync_unlock(service.sync);
        return;
    }
    if (worker->guest_pid)
    {
        /* The creator publishes readiness after publishing the resident. */
        wait_dedicated_work(worker);
    }
    while (!stopping())
    {
        if (dvm_runtime_gc_attach())
        {
            attached = 1;
            break;
        }
        fvm_port_thread_sleep_us(50);
    }
    if (attached)
    {
        worker->identity = (uintptr)context;
        if (worker->guest_pid)
        {
            current_guest = worker;
            if (service.diagnostics)
            {
                fprintf(stderr, "NATIVEGUEST start pid=%d thread=%lu\n",
                    worker->guest_pid, (unsigned long)pthread_self());
            }
        }
        fvm_port_sync_lock(service.sync);
        if (service.ready < service.count - 1)
        {
            service.ready++;
        }
        fvm_port_sync_signal_all(service.sync);
        fvm_port_sync_unlock(service.sync);
        if (waserror())
        {
            DVM_HeapOwnerStats failed_owner;
            fail(up->env->errstr);
            /* A turn-protocol fault cannot safely abandon a live C continuation.
             * Ordinary managed errors retire their claim before returning. */
            if (dvm_native_context_turn(context) || dvm_native_context_process(context) ||
                !dvm_native_owner_statistics(dvm_native_owner_current(),&failed_owner) ||
                failed_owner.phase != DVM_HEAP_ACTIVE)
            {
                fprintf(stderr,"disvm: worker retained an unfinished turn\n");
                exit(1);
            }
            up->prog = nil;
            up->env = &up->defenv;
        }
        else
        {
            while (!stopping() && !worker->guest_retired)
            {
                int progress = service.turn();
                if (progress < 0)
                {
                    fail("managed turn failed");
                    break;
                }
                if (progress == DVM_MANAGED_TURN_GUEST)
                {
                    worker->turns++;
                }
                /* Stop owns the registry through join. A resumed native call
                 * may finish its current turn, but must not reap/mutate links. */
                if (stopping() || worker->guest_retired)
                {
                    break;
                }
                idle_turns = progress ? 0 : idle_turns + 1;
                if (service.automatic && !progress && idle_turns >= 64 && service.live > 1)
                {
                    break;
                }
                worker->handoffs++;
                if (!yield_empty(!progress, 0, NULL))
                {
                    if (!stopping())
                    {
                        error("cannot yield helper scheduler turn");
                    }
                    break;
                }
            }
            poperror();
        }
        if (worker->guest_retired && service.diagnostics)
        {
            fprintf(stderr, "NATIVEGUEST retired pid=%d thread=%lu\n",
                worker->guest_pid, (unsigned long)pthread_self());
        }
        current_guest = NULL;
        if (!dvm_native_owner_statistics(dvm_native_owner_current(),&worker->owner))
        {
            fail("cannot snapshot executor ownership");
        }
        else if (worker->owner.phase == DVM_HEAP_ACTIVE)
        {
            worker->identity = 0;
            service.live--;
        }
        /* A dedicated worker stopped while IDLE leaves its slot for shutdown join. */
        while (!dvm_runtime_gc_detach())
        {
            fvm_port_thread_sleep_us(50);
        }
    }
    if (!dvm_native_context_unbind(context) || !dvm_native_context_destroy(context))
    {
        fail("executor retirement failed");
    }
    fvm_port_sync_lock(service.sync);
    unregister_dedicated_wait(worker);
    worker->done = 1;
    service.generation++;
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
}


/* Allocate one stable slot before creating its OS thread. Caller owns the heap;
 * no wait for attachment is allowed here. Failure leaves registry unchanged. */
static int start_helper(Prog *guest)
{
    ManagedWorker *worker;
    if (stopping() || service.created == ULONG_MAX)
    {
        return 0;
    }
    worker = calloc(1, sizeof(*worker));
    if (!worker)
    {
        return 0;
    }
    worker->id = service.created + 1;
    worker->guest = guest;
    worker->guest_pid = guest ? guest->pid : 0;
    if (guest)
    {
        worker->wake = fvm_port_sync_create();
        if (!worker->wake)
        {
            free(worker);
            return 0;
        }
        fvm_port_sync_lock(service.sync);
        worker->wake_next = service.wake_slots;
        service.wake_slots = worker;
        fvm_port_sync_unlock(service.sync);
    }
    worker->thread = dvm_thread_registrar_start(service.registrar, execute, worker);
    if (!worker->thread)
    {
        if (worker->wake)
        {
            fvm_port_sync_lock(service.sync);
            unregister_dedicated_wait(worker);
            fvm_port_sync_unlock(service.sync);
            fvm_port_sync_destroy(worker->wake);
        }
        free(worker);
        return 0;
    }
    if (guest)
    {
        ((DVM_RuntimeProcess *)guest)->dedicated = worker;
    }
    worker->next = service.helpers;
    service.helpers = worker;
    service.created++;
    service.live++;
    if (service.live + 1 > service.peak)
    {
        service.peak = service.live + 1;
    }
    return 1;
}


/* Publish the service before creating count-1 helpers. automatic enables native
 * wait compensation and idle retirement; limit zero uses available OS resources.
 * diagnostics controls copied lifecycle evidence. turn is the shared dispatcher.
 * cancel_waits wakes suspended native operations after fatal failure.
 * All partial startup threads join before failure returns to the caller. */
int dvm_managed_workers_start(int count, int automatic, unsigned long limit, int diagnostics,
    int (*turn)(void), void (*cancel_waits)(void))
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    int index;
    if (count < 1 || !turn || !cancel_waits || service.sync || !context || !up || up->prog ||
        (limit && (unsigned long)count > limit) ||
        dvm_native_context_process(context) || dvm_native_context_turn(context) ||
        !dvm_native_owner_statistics(dvm_native_owner_current(), &state) ||
        state.phase != DVM_HEAP_ACTIVE || state.pending_progress ||
        !dvm_native_owner_world(dvm_native_owner_current()))
    {
        return 0;
    }
    memset(&service, 0, sizeof(service));
    service.sync = fvm_port_sync_create();
    service.turn = turn;
    service.cancel_waits = cancel_waits;
    service.world = dvm_native_owner_world(dvm_native_owner_current());
    service.awaiting_start = 1;
    service.count = count;
    service.automatic = automatic;
    service.limit = limit;
    service.diagnostics = diagnostics;
    service.peak = 1;
    if (!service.sync)
    {
        return 0;
    }
    service.registrar = dvm_thread_registrar_create();
    if (!service.registrar)
    {
        fvm_port_sync_destroy(service.sync);
        memset(&service, 0, sizeof(service));
        return 0;
    }
    for (index = 0; index < count - 1; index++)
    {
        if (!start_helper(NULL))
        {
            fail("cannot start executor");
            dvm_managed_workers_stop(0);
            return 0;
        }
    }
    return 1;
}


/* Start one OS thread for pid while the creator still owns NativeWorld.
 * No guest pointer is passed to a thread before scheduler publication. */
int dvm_managed_workers_start_guest(Prog *guest)
{
    return service.sync && guest && !stopping() && start_helper(guest);
}


/* Return the PID permanently associated with this dedicated TLS slot.
 * The initial guest uses the controller and supplies its PID at dispatch. */
Prog *dvm_managed_workers_guest(void)
{
    return current_guest ? current_guest->guest : NULL;
}


/* EN: Tell the controller about an extracted reply, after checking the FIFO.
 * RU: Сообщить контроллеру об извлечённом ответе после проверки FIFO. */
int dvm_managed_workers_reply_in_transit(Prog *guest)
{
    ManagedWorker *worker = ((DVM_RuntimeProcess *)guest)->dedicated;
    return worker && __atomic_load_n(&worker->reply_in_transit, __ATOMIC_ACQUIRE);
}


/* EN: Consume this executor's staged reply, if it belongs to guest.
 * RU: Забрать извлечённый ответ текущего исполнителя для guest. */
DVM_OwnerMessage *dvm_managed_workers_take_reply(Prog *guest)
{
    if (!current_guest || current_guest->guest != guest || !current_guest->staged_reply)
    {
        return NULL;
    }
    DVM_OwnerMessage *reply = current_guest->staged_reply;
    current_guest->staged_reply = NULL;
    __atomic_store_n(&current_guest->reply_in_transit, 0, __ATOMIC_RELEASE);
    return reply;
}


/* Latch retirement on the dedicated thread after its Prog has been freed. */
void dvm_managed_workers_guest_retired(int pid)
{
    if (current_guest)
    {
        if (current_guest->guest_pid != pid)
        {
            abort();
        }
        current_guest->guest = NULL;
        current_guest->guest_retired = 1;
    }
}


/* Advance the wake generation. The caller guarantees the service stays
 * live through this signal; no managed pointer crosses it. */
void dvm_managed_workers_wake(void)
{
    if (service.sync)
    {
        fvm_port_sync_lock(service.sync);
        service.generation++;
        fvm_port_sync_signal_all(service.sync);
        fvm_port_sync_unlock(service.sync);
    }
}


/* Publish readiness to the guest's native condition. The caller holds the
 * heap turn, so guest and its dedicated slot remain live through the signal. */
void dvm_managed_workers_wake_guest(Prog *guest)
{
    ManagedWorker *worker = ((DVM_RuntimeProcess *)guest)->dedicated;
    if (worker)
    {
        fvm_port_sync_lock(worker->wake);
        fvm_port_sync_signal_all(worker->wake);
        fvm_port_sync_unlock(worker->wake);
    }
    dvm_managed_workers_wake();
}


/* Mark a native completion before it queues for heap reentry, then consume the
 * mark after admission. arriving is one/zero; return one only when tracked.
 * Native mutex protects this scalar; no managed pointer or heap lock is used. */
int dvm_managed_workers_returning(int arriving)
{
    if (!service.sync || !service.automatic)
    {
        return 0;
    }
    fvm_port_sync_lock(service.sync);
    if (arriving)
    {
        service.returning++;
    }
    else if (service.returning)
    {
        service.returning--;
    }
    else
    {
        fvm_port_sync_unlock(service.sync);
        fprintf(stderr, "disvm: unmatched automatic executor reentry\n");
        exit(1);
    }
    fvm_port_sync_unlock(service.sync);
    return 1;
}


/* Before an audited native wait, retain an executor for other ready work.
 * Caller is ACTIVE and has not withdrawn its current Prog yet. Never grow for
 * protected C stacks. Return a stable diagnostic on resource refusal. The
 * caller decides how to unwind before it withdraws its current Prog. */
const char *dvm_managed_workers_native_wait(void)
{
    Prog *head;
    unsigned long returning;
    if (!service.sync || !service.automatic || stopping())
    {
        return NULL;
    }
    head = dvm_runqueue_head();
    if (!head || !head->link || dvm_runqueue_inflight() + 1 < service.live + 1)
    {
        return NULL;
    }
    fvm_port_sync_lock(service.sync);
    returning = service.returning;
    fvm_port_sync_unlock(service.sync);
    if (returning)
    {
        return NULL;
    }
    if (service.limit && service.live + 1 >= service.limit)
    {
        fprintf(stderr, "disvm: automatic executor resource limit=%lu reached\n", service.limit);
        return "automatic executor resource limit reached";
    }
    if (!start_helper(NULL))
    {
        fprintf(stderr, "disvm: automatic executor OS thread or native allocation failure\n");
        return "automatic executor OS thread or native allocation failure";
    }
    if (service.diagnostics)
    {
        fprintf(stderr, "NATIVEPOOL_GROW executors=%lu created=%lu\n", service.live + 1, service.created);
    }
    return NULL;
}

/* Trace a committed withdrawal under heap exclusion, only in automatic stats
 * mode. No scheduling or ownership change; records distinguish repeated waves. */
void dvm_managed_workers_native_suspended(void)
{
    if (service.sync && service.automatic && service.diagnostics)
    {
        fprintf(stderr, "NATIVEPOOL_SUSPEND inflight=%lu created=%lu\n", dvm_runqueue_inflight(), service.created);
    }
}


/* Yield an empty controller interval; ready tasks stay in the shared FIFO. */
int dvm_managed_workers_yield(int idle)
{
    return !service.sync || yield_empty(idle, 1, NULL);
}


/* Park a rootless non-UI controller when only logical channel waiters remain.
 * Work/retirement generations and stop signals wake it without polling frames. */
int dvm_managed_workers_park(int (*pending)(void))
{
    return !service.sync || yield_empty(1, 0, pending);
}


/* Controller-only progress accounting; no heap or lifecycle transition. */
void dvm_managed_workers_record(void)
{
    if (service.sync)
    {
        fvm_port_sync_lock(service.sync);
        service.controller++;
        fvm_port_sync_unlock(service.sync);
    }
}


/* Record one ordinary file call on an ACTIVE heap owner. Per-handle gates
 * serialize the native cursor outside the heap. Controller reads after joins. */
void dvm_managed_workers_file_call(void)
{
    if (service.sync)
    {
        service.files++;
    }
}


/* The helper never waits on UI while outside its heap turn. Its copied RPC
 * therefore shares the controller's existing single-operation serialization.
 * Check TLS phase before observing the identity protected by that heap gate. */
int dvm_managed_workers_current(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    ManagedWorker *worker;
    if (!context || !dvm_native_context_turn(context) ||
        !dvm_native_owner_statistics(dvm_native_owner_current(), &state) ||
        state.phase != DVM_HEAP_ACTIVE ||
        dvm_native_owner_world(dvm_native_owner_current()) != service.world)
    {
        return 0;
    }
    for (worker = service.helpers; worker; worker = worker->next)
    {
        if (worker->identity == (uintptr)context)
        {
            service.ui_requests++;
            return 1;
        }
    }
    return 0;
}


/* EN: Stop and join helpers. Native slot cleanup runs with controller IDLE.
 * RU: Остановить и присоединить исполнителей. Нативные слоты очищаются в IDLE. */
int dvm_managed_workers_stop(int diagnostics)
{
    int ok;
    ManagedWorker *worker;
    ManagedWorker *next;
    unsigned long before;
    unsigned long turns = 0;
    unsigned long releases = 0;
    unsigned long reentries = 0;
    if (!service.sync)
    {
        return 1;
    }
    before = service.live + 1;
    fvm_port_sync_lock(service.sync);
    service.stopping = 1;
    stop_dedicated_waits();
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
    if (!leave_empty(DVM_NW_SITE_JOIN))
    {
        fprintf(stderr,"disvm: cannot release controller for managed worker join\n");
        exit(1);
    }
    for (worker = service.helpers; worker; worker = worker->next)
    {
        if (!dvm_thread_registrar_join(service.registrar, worker->thread))
        {
            fprintf(stderr, "disvm: cannot join registered worker\n");
            abort();
        }
        turns += worker->turns;
        releases += worker->owner.releases;
        reentries += worker->owner.reentries;
    }
    ok = !service.failed && !service.returning;
    if (diagnostics)
    {
        fprintf(stderr,"NATIVEPOOL executors=%lu joined=1 failed=%d controller_turns=%lu worker_turns=%lu worker_releases=%lu worker_reentries=%lu serialized_files=%lu ui_requests=%lu\n",
            (service.automatic ? service.peak : (unsigned long)service.count),service.failed,service.controller,turns + service.retired_turns,releases + service.retired_releases,reentries + service.retired_reentries,service.files,service.ui_requests);
    }
    if (diagnostics)
    {
        for (worker = service.helpers; worker; worker = worker->next)
        {
            fprintf(stderr, "NATIVEPOOL_WORKER id=%lu joined=1 turns=%lu releases=%lu reentries=%lu handoffs=%lu\n",
                worker->id, worker->turns, worker->owner.releases, worker->owner.reentries, worker->handoffs);
        }
        if (service.automatic)
        {
            fprintf(stderr, "NATIVEPOOL_AUTO created=%lu reaped=%lu live_before_stop=%lu peak=%lu live=0 returning=%lu\n",
                service.created, service.reaped, before, service.peak, service.returning);
        }
    }
    for (worker = service.helpers; worker; worker = next)
    {
        next = worker->next;
        if (worker->wake)
        {
            fvm_port_sync_destroy(worker->wake);
        }
        free(worker);
    }
    dvm_thread_registrar_destroy(service.registrar);
    fvm_port_sync_destroy(service.sync);
    memset(&service, 0, sizeof(service));
    return ok;
}
