/* Copyright (c) 2026 bhgv
 * EN: Cycles queue readiness and admitted pass entry.
 * RU: Готовность очереди Cycles и вход допущенного прохода.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Query committed candidate metadata only; queues survive worker join.
 * RU: Проверить только опубликованные метаданные кандидатов; очереди живут до join. */
int dvm_runtime_rc_candidates_ready(void)
{
    return dvm_retirement_peek(cycle_candidates) != NULL;
}


/* [GC:] EN: Scan queued Cycles candidates on the bound context within budget.
 * Borrow runtime queues/world through the call. Try shared admission without
 * parking peers; preserve refused candidates and return progress or paced retry.
 * RU: Обойти очередь Cycles на привязанном context с бюджетом budget. Очереди
 * и мир заимствованы на весь вызов. Пробовать shared без парковки соседей;
 * сохранять отклонённых кандидатов и вернуть работу или ограниченный повтор. */
unsigned int dvm_runtime_rc_shared_candidates(DVM_NativeContext *context, size_t budget)
{
    DVM_HeapOwner *reader = dvm_native_owner_current();
    if (!budget || !dvm_runtime_rc_candidates_ready() || context != dvm_native_context_current() ||
        !dvm_native_owner_attach_service(reader, world))
    {
        return 0;
    }
    unsigned int completed = 0;
    int retry = 0;
    if (dvm_native_owner_try_enter_idle_shared(reader))
    {
        DVM_RetirementStats queued = {0};
        (void)dvm_retirement_statistics(cycle_candidates, &queued);
        for (size_t index = 0; queued.accepting && index < queued.ready && budget; index++)
        {
            size_t cost;
            int busy;
            if (dvm_rc_scan_candidate(cycle_candidates, &cost, &busy))
            {
                completed++;
            }
            else if (!busy && route_refused_cycle())
            {
                completed++;
            }
            else
            {
                dvm_retirement_rotate(cycle_candidates);
                cycle_candidates_rotated++;
            }
            budget = cost < budget ? budget - cost : 0;
        }
        retry = queued.accepting && dvm_runtime_rc_candidates_ready();
        if (!dvm_native_owner_leave_idle_shared(reader))
        {
            abort();
        }
    }
    else
    { /* The lane retries after releasing its scheduler lease. */
        completed = 1;
    }
    if (!dvm_native_owner_detach(reader))
    {
        abort();
    }
    return completed + retry;
}
