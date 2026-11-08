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


/* EN: Advance a large candidate on context within budget. user is unused.
 * RU: Обработать большого кандидата на context в пределах budget. user не используется. */
unsigned int dvm_gc_duke_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    (void)user;
    return dvm_runtime_rc_shared_duke(context, budget);
}


/* EN: Process one queued graph on context. world and both queues are borrowed.
 * budget enables the pass; return nonzero while queue work remains.
 * RU: Обработать один граф на context. world и очереди заимствованы.
 * budget разрешает проход. Ненулевой результат означает, что в queue есть работа. */
unsigned int dvm_gc_duke_run(DVM_NativeContext *context, size_t budget,
    DVM_NativeWorld *world, DVM_RetirementQueue *queue, DVM_RetirementQueue *fallback)
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
                    (void)dvm_rc_move_candidate(queue, fallback, value);
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
