/* Copyright (c) 2026 bhgv
 * [GC:] EN: Check Fast retry tickets against an in-flight pass and retained backlog.
 * RU: Проверить повтор Fast при незавершённом проходе и непустой резервной очереди.
 */
#include "../host/gc_worker_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Fixture state remains owned by main until service and caller join.
 * RU: Состояние принадлежит main до завершения службы и вызывающего потока. */
typedef struct Fixture
{
    FVM_PortSync *sync;     /* Fixture predicates only. */
    DVM_GCWorkers *workers; /* Owned service through both joins. */
    unsigned calls;         /* Fast callbacks, protected by sync. */
    int release_first;      /* Allow the original callback to finish. */
    int release_backlog;    /* Permit subsequent callbacks to report DONE. */
    int fail;               /* Inject callback failure under sync. */
    int returned;           /* Ticket caller completed beside nonempty backlog. */
} Fixture;


/* EN: Fail on false value with borrowed message; no recovery or ownership transfer.
 * RU: При ложном value завершиться с заимствованным message без восстановления. */
static void require(int value, const char *message)
{
    if (!value)
    {
        fprintf(stderr, "FAIL Fast step: %s\n", message);
        exit(12);
    }
}


/* [GC:] EN: Create a distinct collector context borrowing user until service join.
 * Return null on allocation failure, propagated by service startup.
 * RU: Создать отдельный контекст сборщика с user до join службы.
 * NULL при нехватке памяти передаётся запуску службы. */
static DVM_NativeContext *factory(void *user)
{
    return dvm_native_context_create(sizeof(unsigned long), user);
}


/* [GC:] EN: Run bounded synthetic dispatch work on bound context with budget one.
 * Borrow user; first pass waits for explicit release, later passes retain backlog.
 * RU: Выполнить модель диспетчеризации на привязанном context с budget один.
 * user заимствован; первый проход ждёт разрешения, следующие сохраняют очередь. */
static DVM_GCWorkResult fast(DVM_NativeContext *context, size_t budget, void *user)
{
    Fixture *fixture = user;
    require(context == dvm_native_context_current() && budget == 1, "bound callback");
    require(!dvm_gc_workers_step_fast(fixture->workers), "reject collector self-wait");
    fvm_port_sync_lock(fixture->sync);
    fixture->calls++;
    fvm_port_sync_signal_all(fixture->sync);
    while (fixture->calls == 1 && !fixture->release_first)
    {
        fvm_port_sync_wait(fixture->sync);
    }
    int done = fixture->release_backlog;
    int failed = fixture->fail;
    fvm_port_sync_unlock(fixture->sync);
    return failed ? DVM_GC_FAILED : done ? DVM_GC_DONE : DVM_GC_MORE;
}


/* [GC:] EN: Cycles has no queued work; validate borrowed context/budget/user.
 * RU: У Cycles нет заданий; проверить заимствованные context/budget/user. */
static DVM_GCWorkResult cycles(DVM_NativeContext *context, size_t budget, void *user)
{
    require(context == dvm_native_context_current() && budget == 1 && user, "cycles arguments");
    return DVM_GC_DONE;
}


/* [GC:] EN: Borrow argument through join; await one subsequent attempt while the
 * queue remains nonempty, then publish success under fixture mutex.
 * RU: Заимствовать argument до join; дождаться следующей попытки при непустой
 * очереди и опубликовать успех под мьютексом стенда. */
static void caller(void *argument)
{
    Fixture *fixture = argument;
    require(dvm_gc_workers_step_fast(fixture->workers), "ticket completes");
    fvm_port_sync_lock(fixture->sync);
    require(fixture->calls >= 2 && !fixture->release_backlog, "subsequent attempt before queue exhaustion");
    fixture->returned = 1;
    fvm_port_sync_signal_all(fixture->sync);
    fvm_port_sync_unlock(fixture->sync);
}


/* [GC:] EN: Own world, service and caller through teardown; exercise an active
 * attempt and idle ticket. White-box pending observation controls ordering only,
 * never substitutes collector work. CTest supplies the failure watchdog.
 * RU: Владеть world, службой и вызывающим до очистки; проверить активную попытку
 * и запрос в покое. Наблюдение pending задаёт только порядок, не заменяя работу
 * сборщика. Предел ожидания при отказе задаёт CTest. */
int main(void)
{
    Fixture fixture;
    DVM_GCWorkersConfig config;
    memset(&fixture, 0, sizeof(fixture));
    memset(&config, 0, sizeof(config));
    fixture.sync = fvm_port_sync_create();
    config.world = dvm_world_create();
    require(fixture.sync && config.world, "coordination allocation");
    config.create_context = factory;
    config.user = &fixture;
    config.parallel_shared = 1;
    config.concurrent_fast = fast;
    config.concurrent_cycles = cycles;
    config.Fast.budget = 1;
    config.Cycles.budget = 1;
    config.Fast.pause_us = 100;
    fixture.workers = dvm_gc_workers_create(&config);
    require(fixture.workers && !dvm_gc_workers_step_fast(NULL), "service and invalid request");
    require(dvm_gc_workers_request(fixture.workers, DVM_GC_FAST), "original request");
    fvm_port_sync_lock(fixture.sync);
    while (!fixture.calls)
    {
        fvm_port_sync_wait(fixture.sync);
    }
    fvm_port_sync_unlock(fixture.sync);
    FVM_PortThread *thread = fvm_port_thread_start(caller, &fixture);
    require(thread != NULL, "ticket caller thread");
    int pending = 0;
    while (!pending)
    {
        fvm_port_sync_lock(fixture.workers->sync);
        pending = fixture.workers->Fast.pending;
        fvm_port_sync_unlock(fixture.workers->sync);
        fvm_port_thread_sleep_us(100);
    }
    fvm_port_sync_lock(fixture.sync);
    fixture.release_first = 1;
    fvm_port_sync_signal_all(fixture.sync);
    while (!fixture.returned)
    {
        fvm_port_sync_wait(fixture.sync);
    }
    fixture.release_backlog = 1;
    fvm_port_sync_unlock(fixture.sync);
    fvm_port_thread_join(thread);
    require(dvm_gc_workers_wait_idle(fixture.workers), "backlog release");
    require(dvm_gc_workers_step_fast(fixture.workers), "idle ticket");
    require(dvm_gc_workers_destroy(fixture.workers), "service join");
    fixture.fail = 1;
    fixture.workers = dvm_gc_workers_create(&config);
    require(fixture.workers && !dvm_gc_workers_step_fast(fixture.workers), "callback failure wakes ticket");
    require(!dvm_gc_workers_destroy(fixture.workers), "failed service joins and reports failure");
    require(dvm_world_destroy(config.world), "world destroy");
    fvm_port_sync_destroy(fixture.sync);
    puts("PASS Fast step: active ticket, subsequent pass, retained backlog, idle ticket, self-wait, callback failure");
    return 0;
}
