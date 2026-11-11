/* Copyright (c) 2026 bhgv
 * EN: Rollback of rooted Duke discovery pins.
 * RU: Откат удержаний обхода ДюкGC с корнями.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* EN: Drop a nonfinal pin through the borrowed refcount argument.
 * RU: Снять непоследнее удержание через заимствованный счётчик argument. */
static int drop_duke_pin_nonfinal(void *argument)
{
    return dvm_refcount_drop_nonfinal_u(argument);
}


/* EN: Release up to budget abandoned Duke pins. stopped permits retained
 * destruction. Return nonzero when the journal is empty.
 * RU: Снять до budget удержаний отменённого обхода. stopped разрешает
 * резервное освобождение. Ненулевой результат означает пустой журнал. */
static int release_duke_pins(size_t budget, int stopped)
{
    while (duke_abandoned.pinned_nodes && budget)
    {
        void *value = duke_abandoned.nodes[duke_abandoned.pinned_nodes];
        Heap *heap = D2H(value);
        int transferred = 0;
        int fast_queued = 0;
        if (!stopped && dvm_runtime_root_remove_if(heap, drop_duke_pin_nonfinal, &heap->ref))
        {
            transferred = 1;
        }
        else if (!stopped)
        {
            unsigned long bytes = poolmsize(heapmem, heap);
            if (leaves && !heapmonitor && leaf_eligible(heap))
            {
                if (!defer_leaf(heap, bytes, 0))
                {
                    /* Fast can accept this object after queue pressure clears.
                     * Preserve Duke's reference and root until then. */
                    if (notify_ready)
                    {
                        notify_ready();
                    }
                    return 0;
                }
                __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
                fast_queued = 1;
            }
            else
            {
                DVM_RetirementReservation token = {0};
                if (dvm_retirement_reserve(retirement, bytes, &token) != DVM_RETIRE_OK)
                {
                    __atomic_store_n(&duke_cleanup_fallback, 1, __ATOMIC_RELEASE);
                    if (notify_ready)
                    {
                        notify_ready();
                    }
                    return 0;
                }
                if (!dvm_retirement_commit(retirement, &token, value))
                {
                    abort();
                }
                transferred = 1;
                __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
                if (notify_ready)
                {
                    notify_ready();
                }
            }
        }
        if (!transferred && !dvm_runtime_root_remove(heap))
        {
            abort();
        }
        if (fast_queued && notify_ready)
        {
            notify_ready();
        }
        if (stopped)
        {
            destroy(value);
        }
        duke_abandoned.pinned_nodes--;
        budget--;
    }
    if (duke_abandoned.pinned_nodes)
    {
        return 0;
    }
    free(duke_abandoned.owned_scratch);
    memset(&duke_abandoned, 0, sizeof(duke_abandoned));
    __atomic_store_n(&duke_cleanup_fallback, 0, __ATOMIC_RELEASE);
    return 1;
}
