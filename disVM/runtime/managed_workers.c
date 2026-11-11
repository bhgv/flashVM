/* Copyright (c) 2026 bhgv
 * EN: Lifecycle and wakeups for permanently assigned native process threads.
 * RU: Жизненный цикл и пробуждения закреплённых за процессами нативных потоков.
 */
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
 * Native identity and registry links are published/read under service.sync. */
typedef struct ManagedWorker
{
    struct ManagedWorker *wake_next; /* Registered under service.sync until thread exit. */
    DVM_OwnerMessage *staged_reply; /* Owner-only reply removed before heap admission. */
    int reply_in_transit;       /* Published to the controller until the owner applies the reply. */
    FVM_PortSync *wake;         /* Dedicated readiness condition and generation lock. */
    int startup_committed;      /* Creator has installed the slot, under wake. */
    int wake_stopped;           /* Stop/failure latch under wake. */
    struct ManagedWorker *next; /* Native registry link protected by service.sync. */
    unsigned long id;           /* Stable lifetime ID; never reused while the service exists. */
    Prog *guest;                /* Borrowed until retirement on its own thread. */
    int guest_pid;              /* Dedicated guest identity, or zero for pool execution. */
    int guest_retired;          /* Set by this guest thread after managed retirement. */
    int done;                   /* Published under service.sync after TLS destruction. */
    DVM_ThreadSlot *thread;    /* Registrar handle, joined before this slot is reset. */
    uintptr identity;          /* Context identity under service.sync; zero before context destruction. */
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
    int diagnostics;              /* Emit lifecycle evidence only for --stats. */
    unsigned long live;           /* Live helpers, under heap exclusion. */
    unsigned long peak;           /* Maximum total live executors including controller. */
    unsigned long created;        /* Successfully created helpers. */
    unsigned long next_id;        /* Lifetime IDs allocated by the startup owner. */
    unsigned long generation;     /* Runnable-work publication under sync; prevents lost wakeups. */
    unsigned long reaped;         /* Helpers joined during rootless scheduling. */
    unsigned long retired_turns;  /* Counters accumulated from reaped helper slots. */
    unsigned long retired_releases; /* Audited releases accumulated after join. */
    unsigned long retired_reentries; /* Audited reentries accumulated after join. */
    void (*cancel_waits)(void);    /* Native wakeup hook for a fatal worker failure. */
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
    int wait_for_work;
    fvm_port_sync_lock(service.sync);
    generation = service.generation;
    fvm_port_sync_unlock(service.sync);
    if (pending && pending())
    {
        return !stopping();
    }
    fvm_port_sync_lock(service.sync);
    wait_for_work = idle && !rendezvous;
    while (*slot)
    {
        worker = *slot;
        if (worker->done && !worker->guest)
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
    fvm_port_sync_unlock(service.sync);
    if (!leave_empty(DVM_NW_SITE_IDLE))
    {
        /* The caller still owns the heap. Keep every unjoined slot visible
         * to shutdown even when rootless admission is refused. */
        fvm_port_sync_lock(service.sync);
        while (finished)
        {
            worker = finished;
            finished = worker->next;
            worker->next = service.helpers;
            service.helpers = worker;
        }
        fvm_port_sync_unlock(service.sync);
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
            fprintf(stderr, "NATIVEWORKERS_REAP id=%lu joined=1 turns=%lu\n", worker->id, worker->turns);
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
    ManagedWorker *worker = argument;
    if (worker->guest_pid)
    {
        fvm_port_sync_lock(worker->wake);
        while (!worker->startup_committed)
        {
            fvm_port_sync_wait(worker->wake);
        }
        fvm_port_sync_unlock(worker->wake);
    }
    int cancellation = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    DVM_NativeContext *context = cancellation ? NULL : dvm_native_process_create(NULL);
    int attached = 0;
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
    if (worker->guest_pid && !dvm_runqueue_publish_startup(worker->guest))
    {
        fail("invalid dedicated resident publication");
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
        worker->identity = (uintptr)context;
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
        fvm_port_sync_lock(service.sync);
        worker->identity = 0;
        fvm_port_sync_unlock(service.sync);
        if (!dvm_native_owner_statistics(dvm_native_owner_current(),&worker->owner))
        {
            fail("cannot snapshot executor ownership");
        }
        else if (worker->owner.phase == DVM_HEAP_ACTIVE)
        {
            fvm_port_sync_lock(service.sync);
            service.live--;
            fvm_port_sync_unlock(service.sync);
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


/* EN: Link the native slot while the startup command owns the service gate.
 * RU: Добавить нативный слот, пока команда запуска удерживает шлюз сервиса. */
static void install_worker(ManagedWorker *worker)
{
    service.created++;
    worker->next = service.helpers;
    service.helpers = worker;
    service.live++;
    if (service.live + 1 > service.peak)
    {
        service.peak = service.live + 1;
    }
}


/* EN: Reserve the resident's identity on the native registrar.
 * RU: Зарезервировать идентификатор процесса на нативном регистраторе. */
static int prepare_resident_slot(void *argument)
{
    ManagedWorker *worker = argument;
    if (service.next_id == ULONG_MAX || !dvm_runtime_prepare_resident(worker->guest))
    {
        return 0;
    }
    worker->id = ++service.next_id;
    worker->guest_pid = worker->guest->pid;
    return 1;
}


/* EN: Install the slot, then publish the fully initialized root before replying.
 * RU: Установить слот и опубликовать готовый корень до отправки ответа. */
static void publish_resident_slot(void *argument, DVM_ThreadSlot *thread)
{
    ManagedWorker *worker = argument;
    worker->thread = thread;
    ((DVM_RuntimeProcess *)worker->guest)->dedicated = worker;
    install_worker(worker);
    dvm_runtime_process_root_add(worker->guest);
}


/* EN: Allocate a stable worker slot and start it through the registrar.
 * RU: Выделить стабильный слот и запустить поток через регистратор.
 * Dedicated startup releases admission while waiting for the native handle. */
static int start_helper(Prog *guest)
{
    ManagedWorker *worker;
    if (!guest || stopping())
    {
        return 0;
    }
    worker = calloc(1, sizeof(*worker));
    if (!worker)
    {
        return 0;
    }
    worker->guest = guest;
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

    int released = dvm_runtime_gc_native_try_leave();
    if (released > 0)
    {
        /* Order creation before shutdown's first join request. The new
         * resident waits on its private condition until commit. */
        fvm_port_sync_lock(service.sync);
        if (!service.stopping && !service.failed)
        {
            (void)dvm_thread_registrar_start_prepared(service.registrar, execute, worker,
                prepare_resident_slot, publish_resident_slot);
        }
        fvm_port_sync_unlock(service.sync);
        dvm_runtime_gc_native_enter(released);
    }
    if (!worker->thread)
    {
        fvm_port_sync_lock(service.sync);
        unregister_dedicated_wait(worker);
        fvm_port_sync_unlock(service.sync);
        fvm_port_sync_destroy(worker->wake);
        free(worker);
        return 0;
    }
    fvm_port_sync_lock(worker->wake);
    worker->startup_committed = 1;
    fvm_port_sync_signal_all(worker->wake);
    fvm_port_sync_unlock(worker->wake);
    return 1;
}


/* EN: Start the resident registry on the bound active controller.
 * diagnostics selects lifecycle output. turn and cancel_waits are borrowed
 * callbacks through stop. Return zero on invalid state or native allocation.
 * RU: Запустить реестр закреплённых потоков на активном контроллере.
 * Обратные вызовы заимствованы до stop. При ошибке вернуть ноль. */
int dvm_resident_threads_start(int diagnostics,
    int (*turn)(void), void (*cancel_waits)(void))
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    if (!turn || !cancel_waits || service.sync || !context || !up || up->prog ||
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
    return 1;
}


/* EN: Start a rooted resident, waiting for the registrar outside admission.
 * RU: Запустить укоренённый процесс, ожидая регистратора вне допуска. */
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
    return reply;
}


/* EN: Clear the reply marker only after the owner has applied the reply.
 * RU: Снять отметку ответа только после его обработки владельцем. */
void dvm_managed_workers_finish_reply(Prog *guest)
{
    if (current_guest && current_guest->guest == guest)
    {
        __atomic_store_n(&current_guest->reply_in_transit, 0, __ATOMIC_RELEASE);
    }
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
    int found = 0;
    fvm_port_sync_lock(service.sync);
    for (worker = service.helpers; worker; worker = worker->next)
    {
        if (worker->identity == (uintptr)context)
        {
            service.ui_requests++;
            found = 1;
            break;
        }
    }
    fvm_port_sync_unlock(service.sync);
    return found;
}


/* EN: Stop and join helpers. Native slot cleanup runs with controller IDLE.
 * RU: Остановить и присоединить исполнителей. Нативные слоты очищаются в IDLE. */
int dvm_managed_workers_stop(int diagnostics)
{
    int ok;
    ManagedWorker *worker;
    ManagedWorker *next;
    unsigned long turns = 0;
    unsigned long releases = 0;
    unsigned long reentries = 0;
    if (!service.sync)
    {
        return 1;
    }
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
    ok = !service.failed;
    if (diagnostics)
    {
        fprintf(stderr,"NATIVEWORKERS threads=%lu joined=1 failed=%d controller_turns=%lu worker_turns=%lu worker_releases=%lu worker_reentries=%lu serialized_files=%lu ui_requests=%lu\n",
            service.peak,service.failed,service.controller,turns + service.retired_turns,releases + service.retired_releases,reentries + service.retired_reentries,service.files,service.ui_requests);
    }
    if (diagnostics)
    {
        for (worker = service.helpers; worker; worker = worker->next)
        {
            fprintf(stderr, "NATIVEWORKERS_THREAD id=%lu joined=1 turns=%lu releases=%lu reentries=%lu handoffs=%lu\n",
                worker->id, worker->turns, worker->owner.releases, worker->owner.reentries, worker->handoffs);
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
