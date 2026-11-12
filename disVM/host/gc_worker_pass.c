/* Copyright (c) 2026 bhgv
 * EN: Bounded collector dispatch, with separate concurrent and retained routes.
 * RU: Ограниченные проходы сборки с отдельными конкурентным и резервным путями.
 */
#include "gc_worker_private.h"


/* [GC:] EN: Read shutdown/failure on borrowed workers under its request mutex.
 * No heap admission is held; return nonzero when another pass is forbidden.
 * RU: Проверить остановку/ошибку заимствованной службы workers под её мьютексом.
 * Допуск к куче не захватывается; ненулевой результат запрещает следующий проход. */
static int stopping(DVM_GCWorkers *workers)
{
    int result;
    fvm_port_sync_lock(workers->sync);
    result = workers->stopping || workers->failed;
    fvm_port_sync_unlock(workers->sync);
    return result;
}


/* [GC:] EN: Reserve a retained pass for bound worker only when no other callback
 * is active. Return zero without reservation on contention; never wait for peers.
 * RU: Зарезервировать резервный проход worker, только если других обработчиков
 * нет. При конфликте вернуть ноль без ожидания и без изменения допуска. */
static int try_retained_pass(DVM_GCWorker *worker)
{
    DVM_GCWorkers *workers = worker->owner;
    unsigned int bit = 1U << worker->kind;
    fvm_port_sync_lock(workers->sync);
    int acquired = workers->active == bit;
    if (acquired)
    {
        workers->exclusive = bit;
    }
    fvm_port_sync_unlock(workers->sync);
    return acquired;
}


/* [GC:] EN: Dispatch requested work on the bound worker with no service mutex.
 * Set executed only when a callback runs. Concurrent callbacks return their exact
 * status without world acquisition; retained callbacks pair root stop with resume.
 * Return DONE/MORE/FAILED; borrowed worker and output live through this call.
 * RU: Выполнить requested работу привязанного worker без мьютекса службы.
 * executed отмечает вызов обработчика. Конкурентный путь возвращает точный
 * статус без остановки; резервный парно останавливает/возобновляет мир.
 * Вернуть DONE/MORE/FAILED; worker и выходной параметр живут весь вызов.
 */
DVM_GCWorkResult dvm_gc_worker_collect(DVM_GCWorker *worker, int requested, int *executed)
{
    DVM_GCWorkers *workers = worker->owner;
    DVM_GCPass *pass = worker->kind == DVM_GC_FAST ? &workers->config.Fast : &workers->config.Cycles;
    DVM_GCWorkResult result;
    *executed = 0;
    /* [GC:] EN: A paired concurrent backend is a closed dispatch route. Even
     * DONE or FAILED must never fall through into retained root tracing.
     * RU: Парный конкурентный backend образует замкнутый путь исполнения.
     * Даже DONE и FAILED не переходят к резервной трассировке корней. */
    if (worker->kind != DVM_GC_DUKE && workers->config.concurrent_fast)
    {
        DVM_GCWork callback =
            worker->kind == DVM_GC_FAST ? workers->config.concurrent_fast : workers->config.concurrent_cycles;
        *executed = 1;
        return callback(worker->context, pass->budget, workers->config.user);
    }
    if (worker->kind == DVM_GC_DUKE)
    {
        *executed = 1;
        return workers->config.duke_shared(worker->context, workers->config.duke_budget, workers->config.user)
                   ? DVM_GC_MORE
                   : DVM_GC_DONE;
    }
    if (worker->kind == DVM_GC_CYCLES && workers->config.cycles_shared &&
        workers->config.cycles_shared(worker->context, pass->budget, workers->config.user))
    {
        *executed = 1;
        return DVM_GC_MORE;
    }
    if (!(requested & GC_PASS_REQUEST))
    {
        return DVM_GC_DONE;
    }
    if (worker->kind == DVM_GC_FAST && workers->config.fast_shared)
    {
        unsigned int completed = workers->config.fast_shared(worker->context, pass->budget, workers->config.user);
        if (completed)
        {
            *executed = 1;
            return workers->config.fast_ready && !workers->config.fast_ready(workers->config.user) ? DVM_GC_DONE
                                                                                                   : DVM_GC_MORE;
        }
    }
    if (worker->kind == DVM_GC_FAST && workers->config.fast_ready && !workers->config.fast_ready(workers->config.user))
    {
        return DVM_GC_DONE;
    }
    if (workers->config.parallel_shared && !try_retained_pass(worker))
    {
        return DVM_GC_MORE;
    }
    if (worker->kind == DVM_GC_FAST ? workers->config.fast_quiescent_only : workers->config.cycles_quiescent_only)
    {
        /* [GC:] EN: Work may arrive after an empty shared sample. An admitted
         * guest rules out this quiescent pass; leave new work for shared retry.
         * This is only a hint: the actual acquisition still rechecks admission.
         * RU: После пустого shared-замера может прийти работа. Активный гость
         * исключает quiescent-проход; повторить shared. Захват всё равно
         * проверяет допуск повторно, наблюдение ничего не резервирует. */
        if (worker->kind == DVM_GC_FAST && dvm_world_has_active_guest(workers->config.world))
        {
            return DVM_GC_MORE;
        }
        if (!dvm_world_try_quiescent(workers->config.world, worker->context))
        {
            return DVM_GC_MORE;
        }
    }
    else
    {
        while (!dvm_world_stop_begin(workers->config.world, worker->context))
        {
            if (stopping(workers))
            {
                return DVM_GC_DONE;
            }
            /* Another host collector can own the world; no VM execution lock is held. */
            fvm_port_thread_sleep_us(1000);
        }
    }
    if (!dvm_world_visit(workers->config.world, workers->config.roots, workers->config.user))
    {
        result = DVM_GC_FAILED;
    }
    else
    {
        *executed = 1;
        result = pass->run(worker->context, pass->budget, workers->config.user);
    }
    if (!dvm_world_stop_end(workers->config.world))
    {
        result = DVM_GC_FAILED;
    }
    return result;
}
