/* Copyright (c) 2026 bhgv
 * EN: Native collector thread lifecycle, coalesced requests and observations.
 * RU: Жизненный цикл системных потоков сборки, запросы и наблюдение состояния.
 */
#include "gc_worker_private.h"
#include <stdlib.h>


/* Select one of the explicitly named lane groups.
 * Parameters: workers is a borrowed service; kind is a validated lane ID.
 * Returns: its embedded lane record, never a new allocation.
 * RU: Получить запись потока kind из заимствованной службы workers. Новое владение не создаётся. */
static DVM_GCWorker *lane(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    switch (kind)
    {
    case DVM_GC_FAST:
        return &workers->Fast;
    case DVM_GC_CYCLES:
        return &workers->Cycles;
    default:
        return &workers->Duke;
    }
}


/* Select the first pending lane in rotation while the service mutex is held.
 * RU: Найти первый ожидающий поток workers по очереди; вызывающий держит мьютекс службы. */
static DVM_GCWorker *next_pending(DVM_GCWorkers *workers)
{
    for (unsigned int offset = 0; offset < 3; offset++)
    {
        DVM_GCWorker *worker = lane(workers, (workers->turn + offset) % 3);
        if (worker->pending)
        {
            return worker;
        }
    }
    return NULL;
}


/* Bind one native collector, alternate bounded passes, then release its up.
 * Parameters: argument is the embedded worker record retained through thread join.
 * Returns: nothing. Binds its own context, waits for requests, and releases the
 * context after cooperative shutdown; never frees a still-bound failed context.
 * RU: На записи argument создать и привязать контекст сборщика, обработать запросы, затем снять привязку. Запись живёт до join. */
static void run_worker(void *argument)
{
    DVM_GCWorker *worker = (DVM_GCWorker *)argument;
    DVM_GCWorkers *workers = worker->owner;
    unsigned int bit = 1U << worker->kind;
    int bound;
    worker->context = workers->config.create_context(workers->config.user);
    bound = worker->context && dvm_native_context_bind(worker->context);
    fvm_port_sync_lock(workers->sync);
    workers->started++;
    if (!bound)
    {
        workers->failed = 1;
    }
    fvm_port_sync_signal_all(workers->sync);
    while (bound && !workers->stopping && !workers->failed)
    {
        DVM_GCWorkResult result;
        int executed;
        unsigned long pause_us;
        while (!workers->stopping && !workers->failed &&
               (!worker->pending || workers->exclusive ||
                (!workers->config.parallel_shared && (workers->active || next_pending(workers) != worker))))
        {
            fvm_port_sync_wait(workers->sync);
        }
        if (workers->stopping || workers->failed)
        {
            break;
        }
        int requested = worker->pending;
        worker->pending = 0;
        workers->active |= bit;
        if (workers->config.parallel_shared)
        {
            unsigned int active = __builtin_popcount(workers->active);
            if (active > workers->shared_peak)
            {
                workers->shared_peak = active;
            }
        }
        fvm_port_sync_unlock(workers->sync);
        result = dvm_gc_worker_collect(worker, requested, &executed);
        fvm_port_sync_lock(workers->sync);
        if (executed)
        {
            worker->passes++;
        }
        workers->active &= ~bit;
        workers->exclusive &= ~bit;
        workers->turn = (worker->kind + 1) % 3;
        if (result == DVM_GC_MORE && !workers->stopping)
        {
            worker->pending |= requested;
        }
        else if (result != DVM_GC_DONE && result != DVM_GC_MORE)
        {
            workers->failed = 1;
        }
        fvm_port_sync_signal_all(workers->sync);
        pause_us = worker->kind == DVM_GC_FAST     ? workers->config.Fast.pause_us
                   : worker->kind == DVM_GC_CYCLES ? workers->config.Cycles.pause_us
                                                   : workers->config.duke_pause_us;
        if (result == DVM_GC_MORE && pause_us && !workers->stopping && !workers->failed)
        {
            fvm_port_sync_unlock(workers->sync);
            if (worker->kind == DVM_GC_CYCLES && workers->config.cycles_quiescent_only)
            {
                dvm_world_wait_quiescent_retry(workers->config.world);
            }
            else
            {
                fvm_port_thread_sleep_us(pause_us);
            }
            fvm_port_sync_lock(workers->sync);
        }
    }
    fvm_port_sync_unlock(workers->sync);
    if (bound && !dvm_native_context_unbind(worker->context))
    {
        fvm_port_sync_lock(workers->sync);
        workers->failed = 1;
        fvm_port_sync_unlock(workers->sync);
        /* Preserve a still-bound context instead of freeing roots after a protocol failure. */
        return;
    }
    dvm_native_context_destroy(worker->context);
    worker->context = 0;
}


/* Startup waits only for native context binding, never for a collection request.
 * Parameters: config is borrowed and copied after validation; its world, user
 * and callback-owned root state must outlive all worker joins.
 * Returns: an owned service, or null after startup/validation/allocation failure.
 * RU: Проверить и скопировать config, запустить службу и дождаться привязки контекстов. Вернуть службу или NULL; world/user живут до join. */
DVM_GCWorkers *dvm_gc_workers_create(const DVM_GCWorkersConfig *config)
{
    DVM_GCWorkers *workers;
    int failed;
    if (!config || !config->world || !config->create_context ||
        (!!config->concurrent_fast != !!config->concurrent_cycles) ||
        (config->concurrent_fast && !config->parallel_shared) ||
        (!config->concurrent_fast && (!config->roots || !config->Fast.run || !config->Cycles.run)) ||
        !config->Fast.budget || !config->Cycles.budget || config->Fast.pause_us > DVM_GC_MAX_PAUSE_US ||
        config->Cycles.pause_us > DVM_GC_MAX_PAUSE_US ||
        (config->duke_shared && (!config->duke_budget || config->duke_pause_us > DVM_GC_MAX_PAUSE_US)))
    {
        return 0;
    }
    workers = (DVM_GCWorkers *)calloc(1, sizeof(*workers));
    if (!workers)
    {
        return 0;
    }
    workers->config = *config;
    workers->sync = fvm_port_sync_create();
    if (!workers->sync)
    {
        free(workers);
        return 0;
    }
    workers->Fast.owner = workers;
    workers->Fast.kind = DVM_GC_FAST;
    workers->Cycles.owner = workers;
    workers->Cycles.kind = DVM_GC_CYCLES;
    workers->Fast.thread = fvm_port_thread_start(run_worker, &workers->Fast);
    workers->Cycles.thread = fvm_port_thread_start(run_worker, &workers->Cycles);
    if (config->duke_shared)
    {
        workers->Duke.owner = workers;
        workers->Duke.kind = DVM_GC_DUKE;
        workers->Duke.thread = fvm_port_thread_start(run_worker, &workers->Duke);
    }
    fvm_port_sync_lock(workers->sync);
    if (!workers->Fast.thread || !workers->Cycles.thread || (config->duke_shared && !workers->Duke.thread))
    {
        workers->failed = 1;
    }
    while (!workers->failed && workers->started != 2U + (config->duke_shared != NULL))
    {
        fvm_port_sync_wait(workers->sync);
    }
    failed = workers->failed;
    fvm_port_sync_unlock(workers->sync);
    if (failed)
    {
        dvm_gc_workers_destroy(workers);
        return 0;
    }
    return workers;
}


/* Requests carry no unrooted object address and can be coalesced freely.
 * Parameters: workers is borrowed; kind selects Fast or Cycles. No object pointer
 * is enqueued. Backend code must separately root any deferred managed references.
 * Returns: one after coalescing a request, zero for invalid or stopped/failed state.
 * RU: Объединить биты requested для потока kind службы workers под мьютексом. Вернуть ноль при неверном запросе, остановке или ошибке. */
static int request(DVM_GCWorkers *workers, DVM_GCKind kind, int requested)
{
    if (!workers ||
        (kind != DVM_GC_FAST && kind != DVM_GC_CYCLES && (kind != DVM_GC_DUKE || !workers->config.duke_shared)))
    {
        return 0;
    }
    fvm_port_sync_lock(workers->sync);
    if (workers->stopping || workers->failed)
    {
        fvm_port_sync_unlock(workers->sync);
        return 0;
    }
    lane(workers, kind)->pending |= requested;
    fvm_port_sync_signal_all(workers->sync);
    fvm_port_sync_unlock(workers->sync);
    return 1;
}


/* [GC:] EN: Coalesce a full pass request for kind on borrowed workers; return
 * zero for invalid/stopped/failed state. The backend separately roots its work.
 * RU: Объединить запрос полного прохода kind в workers; при неверном состоянии,
 * остановке или ошибке вернуть ноль. Корни заданий обеспечивает backend. */
int dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    return request(workers, kind, GC_PASS_REQUEST);
}


/* [GC:] EN: Wake Cycles candidate processing on borrowed workers. Return one
 * if accepted, zero on invalid/stopped/failed state; no object pointer is queued.
 * RU: Запросить обход кандидатов Cycles службы workers. Вернуть один при приёме
 * либо ноль при отказе; указатели на объекты в запрос не передаются. */
int dvm_gc_workers_request_candidates(DVM_GCWorkers *workers)
{
    return request(workers, DVM_GC_CYCLES, GC_CANDIDATE_REQUEST);
}


/* Caller must have left any mutator interval needed by the collector.
 * Parameters: workers is borrowed; the caller must leave its mutator interval first.
 * Returns: one when the requested lanes are idle, zero for invalid/self-worker calls or
 * shutdown/failure. This call blocks and does not impose a collection timeout.
 * RU: Ждать покоя службы workers либо только Fast при fast_only. Самоожидание и ошибка дают ноль; в резервном режиме вызывающий заранее выходит из допуска. */
static int wait_idle(DVM_GCWorkers *workers, int fast_only)
{
    int result;
    if (!workers || fvm_port_thread_is_current(workers->Fast.thread) ||
        fvm_port_thread_is_current(workers->Cycles.thread) ||
        (workers->Duke.thread && fvm_port_thread_is_current(workers->Duke.thread)))
    {
        return 0;
    }
    fvm_port_sync_lock(workers->sync);
    while (!workers->failed && !workers->stopping &&
           (workers->Fast.pending || (workers->active & (1U << DVM_GC_FAST)) ||
            (!fast_only && (workers->active || workers->Cycles.pending || workers->Duke.pending))))
    {
        fvm_port_sync_wait(workers->sync);
    }
    result = !workers->failed && !workers->stopping;
    fvm_port_sync_unlock(workers->sync);
    return result;
}


/* Wait for all configured lanes before collector shutdown.
 * RU: Дождаться всех потоков workers; вернуть ноль при ошибке или запрещённом самоожидании. Владение службой не передаётся. */
int dvm_gc_workers_wait_idle(DVM_GCWorkers *workers)
{
    return wait_idle(workers, 0);
}


/* Wait only for Fast work while a guest has withdrawn from the heap.
 * RU: Дождаться только Fast службы workers без передачи владения; вернуть ноль при ошибке или самоожидании. */
int dvm_gc_workers_wait_fast_idle(DVM_GCWorkers *workers)
{
    return wait_idle(workers, 1);
}


/* Observation never waits for an active backend callback to complete.
 * Parameters: workers is borrowed; out receives a caller-owned coherent snapshot.
 * Returns: one after copying state under the mutex, zero for null arguments.
 * The snapshot does not reserve the observed state against later requests.
 * RU: Скопировать согласованное состояние workers в заимствованный out под мьютексом. Неверные указатели дают ноль; проход не ожидается. */
int dvm_gc_workers_state(DVM_GCWorkers *workers, DVM_GCWorkersState *out)
{
    if (!workers || !out)
    {
        return 0;
    }
    fvm_port_sync_lock(workers->sync);
    out->Fast.passes = workers->Fast.passes;
    out->Fast.pending = workers->Fast.pending != 0;
    out->Cycles.passes = workers->Cycles.passes;
    out->Cycles.pending = workers->Cycles.pending != 0;
    out->Duke.passes = workers->Duke.passes;
    out->Duke.pending = workers->Duke.pending != 0;
    out->active = workers->active ? __builtin_ctz(workers->active) + 1 : 0;
    out->shared_peak = workers->shared_peak;
    out->stopping = workers->stopping;
    out->failed = workers->failed;
    fvm_port_sync_unlock(workers->sync);
    return 1;
}


/* Stop only cooperatively; never destroy a thread that is still using heap roots.
 * Parameters: workers transfers ownership for cooperative shutdown and joining.
 * Returns: one for successful shutdown or null; zero for a latched failure or a
 * rejected self-join. A self-join rejection retains the service; other paths free it.
 * RU: Остановить workers, выполнить join и освободить службу. Самоожидание отвергается без освобождения; ошибка службы возвращает ноль. */
int dvm_gc_workers_destroy(DVM_GCWorkers *workers)
{
    int result;
    if (!workers)
    {
        return 1;
    }
    if ((workers->Fast.thread && fvm_port_thread_is_current(workers->Fast.thread)) ||
        (workers->Cycles.thread && fvm_port_thread_is_current(workers->Cycles.thread)) ||
        (workers->Duke.thread && fvm_port_thread_is_current(workers->Duke.thread)))
    {
        return 0;
    }
    fvm_port_sync_lock(workers->sync);
    workers->stopping = 1;
    workers->Fast.pending = 0;
    workers->Cycles.pending = 0;
    workers->Duke.pending = 0;
    fvm_port_sync_signal_all(workers->sync);
    fvm_port_sync_unlock(workers->sync);
    if (workers->Fast.thread)
    {
        fvm_port_thread_join(workers->Fast.thread);
    }
    if (workers->Cycles.thread)
    {
        fvm_port_thread_join(workers->Cycles.thread);
    }
    if (workers->Duke.thread)
    {
        fvm_port_thread_join(workers->Duke.thread);
    }
    result = !workers->failed;
    fvm_port_sync_destroy(workers->sync);
    free(workers);
    return result;
}
