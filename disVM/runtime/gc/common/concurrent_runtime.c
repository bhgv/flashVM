/* Copyright (c) 2026 bhgv
 * [GC:] EN: Native-only collector dispatch and pre-close concurrent draining.
 * RU: Строго конкурентные проходы сборщиков и обработка очередей перед закрытием.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "../collector_passes.h"
#include "runtime_state.h"
#include "../../runtime_rc.h"
#include "../../runtime_gc.h"
#include "../../runqueue.h"
#include "disvm/native_owner.h"


/* [GC:] EN: Run shared Fast on borrowed context/budget/user and validate its
 * remaining native work. Return MORE until queues empty; never claim exclusion.
 * RU: Выполнить shared Fast с заимствованными context/budget/user и проверить
 * остаток. Вернуть MORE до опустошения очередей; эксклюзив не захватывается. */
DVM_GCWorkResult dvm_gc_fast_concurrent(DVM_NativeContext *context, size_t budget, void *user)
{
    (void)dvm_gc_fast_shared(context, budget, user);
    dvm_runtime_rc_check_concurrent();
    return dvm_gc_fast_ready(user) ? DVM_GC_MORE : DVM_GC_DONE;
}


/* [GC:] EN: Scan shared candidates using borrowed context/budget/user. Real
 * shared retry requests MORE; an empty lane is DONE without a full heap trace.
 * RU: Обойти shared-кандидатов с заимствованными context/budget/user. Настоящий
 * повтор требует MORE; пустой поток даёт DONE без общего обхода кучи. */
DVM_GCWorkResult dvm_gc_cycles_concurrent(DVM_NativeContext *context, size_t budget, void *user)
{
    unsigned int work = dvm_gc_cycles_shared(context, budget, user);
    dvm_runtime_rc_check_concurrent();
    return work ? DVM_GC_MORE : DVM_GC_DONE;
}


/* [GC:] EN: With all guests joined, leave an ACTIVE controller temporarily so
 * its last reader epoch can finish. Wake all lanes, wait for shared work, then
 * restore its original phase. Keep queues accepting through the whole drain.
 * Reject wrong controller/world/phase, live guards/progress or in-flight guests
 * before any request or phase mutation. Return zero on failure; never reload
 * registers from Prog.R or discard owners.
 * RU: После join гостей временно вывести ACTIVE-контроллер для завершения
 * последней эпохи. Разбудить все службы, дождаться shared-работы и вернуть фазу.
 * Очереди остаются принимающими. Чужой контроллер/мир/фаза, живые guards/progress
 * либо выполняющиеся гости отвергаются до запросов и изменения фазы. Ноль
 * при ошибке; Prog.R не восстанавливается,
 * владельцы не отбрасываются. */
int dvm_runtime_gc_concurrent_drain(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    DVM_NativeContext *context = dvm_native_context_current();
    if (!dvm_gc_runtime.world || context != dvm_gc_runtime.owner ||
        dvm_native_owner_world(owner) != dvm_gc_runtime.world || !dvm_native_owner_statistics(owner, &state) ||
        (state.phase != DVM_HEAP_ACTIVE && state.phase != DVM_HEAP_IDLE) || state.pending_progress ||
        dvm_native_context_object_count(context) || dvm_runqueue_inflight())
    {
        return 0;
    }
    int active = state.phase == DVM_HEAP_ACTIVE;
    if (active && !dvm_native_owner_leave(owner))
    {
        return 0;
    }
    int result = dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_FAST) &&
                 dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_CYCLES) &&
                 dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_DUKE) &&
                 dvm_gc_workers_wait_idle(dvm_gc_runtime.workers);
    if (active && !dvm_native_owner_enter(owner))
    {
        return 0;
    }
    return result;
}
