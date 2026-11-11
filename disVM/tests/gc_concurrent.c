/* Copyright (c) 2026 bhgv
 * EN: Native collector dispatch while permanent mutators never poll safepoints.
 * RU: Диспетчер сборки при активных исполнителях без опроса safepoint.
 */
#include "disvm/gc_workers.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MUTATOR_COUNT 2 /* Permanent active owners for the entire collection. */
#define PASS_COUNT 8    /* Required bounded slices on each collector lane. */
#define PASS_BUDGET 3   /* Backend work units passed unchanged by the dispatcher. */
#define WAIT_STEPS 5000 /* Diagnostic deadline, not a scheduling assumption. */


/* [GC:] EN: Own native coordination through all joins; no managed heap is mocked.
 * RU: Владеть нативным состоянием до join; управляемая куча не подменяется. */
typedef struct Backend
{
    FVM_PortSync *sync;      /* Guards counters and test handshakes only. */
    DVM_NativeWorld *world;  /* Owned until every participant detaches. */
    DVM_GCWorkers *workers;  /* Borrowed by callbacks, owned by main. */
    unsigned int factories;  /* Contexts actually created under sync. */
    unsigned int mutators;   /* Number of resident active owners. */
    unsigned int entered;    /* Collector first-pass arrival bits. */
    unsigned int fast;       /* Concurrent Fast callback count. */
    unsigned int cycles;     /* Concurrent Cycles callback count. */
    unsigned long heartbeat; /* Mutator progress independent of collector callbacks. */
    int release_callbacks;   /* Main verified simultaneous admission. */
    int release_mutators;    /* Collection completed before residents may leave. */
    int fail_fast;           /* Inject FAILED after the first completed slice. */
} Backend;


/* EN: Require condition; message identifies the violated protocol. Exit on failure.
 * RU: Проверить condition; message описывает нарушение. При ошибке завершиться. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL concurrent GC: %s\n", message);
        exit(1);
    }
}


/* [GC:] EN: Allocate a bound-later native context; user remains borrowed.
 * RU: Создать нативный контекст для последующей привязки; user заимствован. */
static DVM_NativeContext *create_context(void *user)
{
    Backend *backend = user;
    fvm_port_sync_lock(backend->sync);
    backend->factories++;
    fvm_port_sync_unlock(backend->sync);
    return dvm_native_context_create(sizeof(unsigned long), NULL);
}


/* [GC:] EN: Keep one permanent owner active without polling until main releases it.
 * argument is borrowed Backend; detach and destroy all thread-owned native state.
 * RU: Держать постоянного владельца активным без опроса до разрешения main.
 * argument — заимствованный Backend; отсоединить и удалить состояние потока. */
static void mutator(void *argument)
{
    Backend *backend = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(unsigned long), NULL);
    check(context && dvm_native_context_bind(context), "mutator bind");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_attach(owner, backend->world), "mutator attach");
    check(dvm_native_owner_enter(owner), "mutator enter");
    fvm_port_sync_lock(backend->sync);
    backend->mutators++;
    while (!backend->release_mutators)
    {
        backend->heartbeat++;
        fvm_port_sync_unlock(backend->sync);
        fvm_port_thread_sleep_us(100);
        fvm_port_sync_lock(backend->sync);
    }
    fvm_port_sync_unlock(backend->sync);
    check(dvm_native_owner_leave(owner), "mutator leave");
    check(dvm_native_owner_detach(owner), "mutator detach");
    check(dvm_native_context_unbind(context), "mutator unbind");
    check(dvm_native_context_destroy(context), "mutator destroy");
}


/* [GC:] EN: Run one bounded lane slice. context is bound, user is borrowed Backend;
 * budget must match configuration. First slices overlap behind a native handshake.
 * Return MORE/DONE or injected FAILED after releasing all admission and locks.
 * RU: Выполнить порцию lane на context с budget и заимствованным Backend user.
 * Первые порции перекрываются через нативное рукопожатие. После освобождения
 * допуска и мьютексов вернуть MORE/DONE либо внедрённый FAILED. */
static DVM_GCWorkResult slice(DVM_NativeContext *context, size_t budget, void *user, int lane)
{
    Backend *backend = user;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(context == dvm_native_context_current() && budget == PASS_BUDGET, "callback identity/budget");
    check(dvm_native_owner_attach_service(owner, backend->world), "collector attach");
    check(dvm_native_owner_try_enter_idle_shared(owner), "collector shared entry");
    fvm_port_sync_lock(backend->sync);
    unsigned int count = lane == DVM_GC_FAST ? ++backend->fast : ++backend->cycles;
    if (count == 1)
    {
        backend->entered |= 1U << lane;
        fvm_port_sync_signal_all(backend->sync);
        while (!backend->release_callbacks)
        {
            fvm_port_sync_wait(backend->sync);
        }
    }
    int fail = backend->fail_fast && lane == DVM_GC_FAST && count == 2;
    fvm_port_sync_unlock(backend->sync);
    check(dvm_native_owner_leave_idle_shared(owner), "collector shared leave");
    check(dvm_native_owner_detach(owner), "collector detach");
    return fail ? DVM_GC_FAILED : count < PASS_COUNT ? DVM_GC_MORE : DVM_GC_DONE;
}


/* [GC:] EN: Fast wrapper for bound context, budget and borrowed user.
 * RU: Обработчик Fast для привязанного context, budget и заимствованного user. */
static DVM_GCWorkResult fast(DVM_NativeContext *context, size_t budget, void *user)
{
    return slice(context, budget, user, DVM_GC_FAST);
}


/* [GC:] EN: Cycles wrapper for bound context, budget and borrowed user.
 * RU: Обработчик Cycles для привязанного context, budget и заимствованного user. */
static DVM_GCWorkResult cycles(DVM_NativeContext *context, size_t budget, void *user)
{
    return slice(context, budget, user, DVM_GC_CYCLES);
}


/* [GC:] EN: Fail if retained dispatch is reached with any borrowed arguments.
 * RU: Завершиться при попадании в резервный путь с любыми аргументами. */
static DVM_GCWorkResult forbidden(DVM_NativeContext *context, size_t budget, void *user)
{
    (void)context;
    (void)budget;
    (void)user;
    check(0, "retained fallback executed");
    return DVM_GC_FAILED;
}


/* [GC:] EN: Fail if a retained roots visitor receives any borrowed snapshot.
 * RU: Завершиться при вызове резервного посетителя корней с любым снимком. */
static void forbidden_roots(DVM_NativeContext *context, void *registers, size_t size, void *process, void *user)
{
    (void)context;
    (void)registers;
    (void)size;
    (void)process;
    (void)user;
    check(0, "retained root visitor executed");
}


/* EN: Wait for expected phase under backend's native mutex. No mutator polls.
 * Detect a stop request explicitly before a missing callback can time out.
 * RU: Дождаться фазы expected под нативным мьютексом backend, без опроса VM.
 * Запрос остановки обнаружить до таймаута отсутствующего обработчика. */
static void wait_phase(Backend *backend, int collectors)
{
    for (unsigned int step = 0; step < WAIT_STEPS; step++)
    {
        DVM_WorldState state;
        check(dvm_world_state(backend->world, &state), "world observation");
        check(!state.stopping && !state.stopped && !state.parked, "global stop requested");
        fvm_port_sync_lock(backend->sync);
        int ready = collectors ? backend->entered == 3 : backend->mutators == MUTATOR_COUNT;
        fvm_port_sync_unlock(backend->sync);
        if (ready)
        {
            return;
        }
        fvm_port_thread_sleep_us(1000);
    }
    check(0, "native handshake timed out");
}


/* [GC:] EN: Execute one lifecycle with live residents, optional failure and null
 * retained pointers. Verify startup rejection before factories and joined teardown.
 * RU: Проверить жизнь службы с активными исполнителями, отказом и пустыми
 * резервными указателями; отказ конфигурации до создания контекстов и join. */
static void run(int locked, int fail, int null_retained)
{
    Backend backend;
    DVM_GCWorkersConfig config;
    FVM_PortThread *threads[MUTATOR_COUNT];
    memset(&backend, 0, sizeof(backend));
    memset(&config, 0, sizeof(config));
    backend.sync = fvm_port_sync_create();
    backend.world = dvm_world_create_serial(locked);
    backend.fail_fast = fail;
    check(backend.sync && backend.world, "native allocation");
    config.world = backend.world;
    config.create_context = create_context;
    config.user = &backend;
    config.Fast.budget = PASS_BUDGET;
    config.Cycles.budget = PASS_BUDGET;
    config.concurrent_fast = fast;
    check(!dvm_gc_workers_create(&config), "unpaired backend accepted");
    config.concurrent_cycles = cycles;
    check(!dvm_gc_workers_create(&config), "serialized concurrent backend accepted");
    config.parallel_shared = 1;
    check(backend.factories == 0, "invalid configuration started a thread");
    if (!null_retained)
    {
        config.Fast.run = forbidden;
        config.Cycles.run = forbidden;
        config.roots = forbidden_roots;
    }
    backend.workers = dvm_gc_workers_create(&config);
    check(backend.workers != NULL && backend.factories == 2, "concurrent startup");
    for (unsigned int index = 0; index < MUTATOR_COUNT; index++)
    {
        threads[index] = fvm_port_thread_start(mutator, &backend);
        check(threads[index] != NULL, "mutator thread startup");
    }
    wait_phase(&backend, 0);
    check(dvm_gc_workers_request(backend.workers, DVM_GC_FAST), "request Fast");
    check(dvm_gc_workers_request_candidates(backend.workers), "request Cycles candidates");
    wait_phase(&backend, 1);
    DVM_WorldState world;
    check(dvm_world_state(backend.world, &world) && world.active == MUTATOR_COUNT + 2,
          "both collectors must overlap both permanent mutators");
    fvm_port_sync_lock(backend.sync);
    unsigned long before = backend.heartbeat;
    fvm_port_sync_unlock(backend.sync);
    /* Enqueue while callbacks run; their completion must preserve pending work. */
    check(dvm_gc_workers_request(backend.workers, DVM_GC_FAST), "in-flight Fast request");
    check(dvm_gc_workers_request(backend.workers, DVM_GC_CYCLES), "in-flight Cycles request");
    fvm_port_thread_sleep_us(10000);
    fvm_port_sync_lock(backend.sync);
    check(backend.heartbeat > before, "mutators stopped during collector overlap");
    backend.release_callbacks = 1;
    fvm_port_sync_signal_all(backend.sync);
    fvm_port_sync_unlock(backend.sync);
    check(dvm_gc_workers_wait_idle(backend.workers) == !fail, "idle/failure result");
    DVM_GCWorkersState workers;
    check(dvm_gc_workers_state(backend.workers, &workers), "worker state");
    check(workers.failed == fail && workers.shared_peak >= 2, "overlap/failure state");
    if (!fail)
    {
        check(workers.Fast.passes >= PASS_COUNT && workers.Cycles.passes >= PASS_COUNT,
              "MORE lost a bounded continuation");
        check(dvm_gc_workers_request_candidates(backend.workers) && dvm_gc_workers_wait_idle(backend.workers),
              "idle service did not wake again");
    }
    check(dvm_gc_workers_destroy(backend.workers) == !fail, "joined collector result");
    check(dvm_world_state(backend.world, &world) && world.active == MUTATOR_COUNT && !world.stopping &&
              !world.stopped && !world.parked,
          "residents must remain active after join");
    fvm_port_sync_lock(backend.sync);
    backend.release_mutators = 1;
    fvm_port_sync_unlock(backend.sync);
    for (unsigned int index = 0; index < MUTATOR_COUNT; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_world_destroy(backend.world), "world leaked participants");
    fvm_port_sync_destroy(backend.sync);
}


/* EN: Run both poll backends, retained-pointer forms and failure outcomes.
 * RU: Проверить оба режима опроса, формы резервных указателей и исходы ошибок. */
int main(void)
{
    for (int locked = 0; locked < 2; locked++)
    {
        for (int fail = 0; fail < 2; fail++)
        {
            for (int null_retained = 0; null_retained < 2; null_retained++)
            {
                run(locked, fail, null_retained);
            }
        }
    }
    puts("PASS concurrent GC dispatch: no mutator polls, no global stop, bounded retry, failure and joined lifetimes");
    return 0;
}
