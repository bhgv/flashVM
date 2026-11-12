/* Copyright (c) 2026 bhgv
 * EN: Epoch-gated shared leaf and graph reclamation.
 * RU: Параллельное освобождение листьев и графов после эпохи.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */
static int retirement_ready(int include_shared_retries); /* Queue metadata only. */


/* [GC:] EN: Run bounded shared Fast work on the bound context, with at most budget visits/claims per category. No
 * blocking admission or cycle discovery occurs here. Keep roots on allocation/claim refusal; return
 * progress or an epoch retry to avoid a stopped fallback while readers remain.
 * RU: Выполнить ограниченный shared Fast на привязанном context с бюджетом budget просмотров/захватов по
 * категориям. Здесь нет ожидания допуска или обхода циклов. При отказе выделения/захвата корни сохраняются;
 * вернуть работу или повтор эпохи вместо остановленного fallback при живых читателях. */
unsigned int dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    if (!dvm_retirement_peek(leaves) && !dvm_retirement_peek(frames) && !dvm_retirement_peek(record_retries) &&
        !dvm_retirement_peek(retirement))
    {
        return 0;
    }
    DVM_HeapOwner *reader = dvm_native_owner_current();
    if (context != dvm_native_context_current() || !dvm_native_owner_attach_service(reader, world))
    {
        return 0;
    }
    unsigned int completed = 0;
    int retry = 0;
    if (dvm_native_owner_try_enter_idle_shared(reader))
    {
        size_t shares[FastQueueCount];
        fast_work_budget(budget, shares);
        if (!heapmonitor)
        {
            completed += drain_shared_owners(shares[FastOwners], &retry);
            completed += drain_shared_leaf_queue(record_retries, shares[FastRetries], &retry);
            completed += drain_shared_leaf_queue(leaves, shares[FastLeaves], &retry);
        }
        if (!heapmonitor)
        {
            completed += drain_shared_children(shares[FastFrames], &retry);
            /* An accepting retry queue is outside the exclusive drain.
             * Ask for another paced shared pass if nothing else can run. */
            DVM_RetirementStats pending = {0};
            (void)dvm_retirement_statistics(record_retries, &pending);
            retry |= !completed && pending.accepting && pending.ready && !retirement_ready(0) &&
                     !dvm_managed_owner_pending();
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
    return completed + retry;
}
