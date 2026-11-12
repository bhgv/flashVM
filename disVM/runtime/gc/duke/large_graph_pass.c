/* Copyright (c) 2026 bhgv
 *
 * EN: Duke GC callback for its queue of large graphs.
 * RU: Проход ДюкGC по отдельной очереди больших графов.
 */
#include "../collector_passes.h"
#include "../common/candidate_pass.h"
#include "disvm/native_owner.h"
#include <stdlib.h>
#include "../../runtime_rc.h"


/* [GC:] EN: Adapt bound context/budget to the runtime-owned Duke queues.
 * user is unused. Return pending work/retry; no world stop or owner transfer here.
 * RU: Передать привязанный context/budget очередям Duke рантайма. user не нужен.
 * Вернуть наличие работы/повтора; остановки мира и передачи владельцев здесь нет. */
unsigned int dvm_gc_duke_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    (void)user;
    return dvm_runtime_rc_shared_duke(context, budget);
}


/* [GC:] EN: Process one candidate/suspended slice on bound context when budget
 * is positive. world, queue and fallback survive worker joins. Try shared admission
 * without parking guests; preserve owner/root on refusal and rotate failed fallback
 * moves. Successful suspended discovery keeps its head. Return pending/retry, or
 * zero for empty/closed work, zero budget or foreign context; protocol faults abort.
 * RU: Обработать одного кандидата/порцию на привязанном context при budget > 0.
 * world, queue и fallback живут до join. Пробовать shared без парковки гостей;
 * сохранять ссылку/корень при отказе и перемещать голову после отказа передачи
 * в резерв. Приостановленный обход удерживает голову. Вернуть работу/повтор либо
 * ноль при пустой/закрытой очереди, нулевом бюджете или чужом контексте; сбой
 * протокола вызывает abort. */
unsigned int dvm_gc_duke_run(DVM_NativeContext *context,
                             size_t budget,
                             DVM_NativeWorld *world,
                             DVM_RetirementQueue *queue,
                             DVM_RetirementQueue *fallback)
{
    DVM_RetirementStats queued = {0};
    (void)dvm_retirement_statistics(queue, &queued);
    DVM_HeapOwner *reader = dvm_native_owner_current();
    if (!budget || !queued.ready || !queued.accepting || context != dvm_native_context_current())
    {
        return 0;
    }
    if (!dvm_native_owner_attach_service(reader, world))
    {
        return 1;
    }
    if (dvm_native_owner_try_enter_idle_shared(reader))
    {
        size_t cost;
        int busy;
        if (!dvm_rc_scan_candidate(queue, &cost, &busy))
        {
            if (busy)
            {
                dvm_retirement_rotate(queue);
            }
            else
            {
                void *value = dvm_retirement_peek(queue);
                if (value)
                {
                    if (!dvm_rc_move_candidate(queue, fallback, value))
                    {
                        /* [GC:] EN: A failed reservation preserves this head's
                         * owner/root. Let an independent graph run next; only
                         * a suspended discovery slice may pin the queue head.
                         * RU: Отказ резерва сохраняет владельца/корень головы.
                         * Пропустить независимый граф; удерживать голову может
                         * только незавершённая порция обхода. */
                        dvm_retirement_rotate(queue);
                    }
                }
            }
        }
        if (!dvm_native_owner_leave_idle_shared(reader))
        {
            abort();
        }
    }
    if (!dvm_native_owner_detach(reader))
    {
        abort();
    }
    return dvm_retirement_peek(queue) != NULL;
}
