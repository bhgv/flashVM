/* Copyright (c) 2026 bhgv
 * EN: Mandatory release publication by admitted producers.
 * RU: Обязательная передача ссылок допущенными производителями.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] Transfer the caller's reference to GC. Optional leaf and cycle
 * lanes may decline, but a live mutator always publishes to the release FIFO.
 * RU: Передать ссылку вызывающего в GC. Специализированные очереди могут
 * отказаться, но действующий исполнитель всегда публикует её в FIFO. */
int dvm_runtime_rc_defer(void *value)
{
    if (!admitted(1) || !accepting)
    {
        return 0;
    }
    int active = admitted(0);
    int helper = dvm_native_context_current() != owner;
    Heap *heap = D2H(value);
    int candidate = record_candidate(heap);
    if (!candidate && dvm_refcount_drop_nonfinal_u(&heap->ref))
    {
        return 1;
    }
    int last = dvm_refcount_load_u(&heap->ref) == 1;
    unsigned long bytes = poolmsize(heapmem, heap);
    if (!last || !defer_leaf(heap, bytes, active))
    {
        DVM_RetirementReservation token = {0};
        DVM_HeapKey key = {0};
        int ready = 0;
        int queued = 0;
        if (!last && candidate)
        {
            key = dvm_heap_pool_key(heapmem, heap);
        }
        /* Transfer the existing strong reference and root it for tracing. */
        if (!dvm_runtime_root_add(heap))
        {
            fprintf(stderr, "disvm: out of memory rooting mandatory GC release\n");
            abort();
        }
        Setmark(heap);
        if (candidate_transfer && key.generation &&
            dvm_retirement_reserve(cycle_candidates, bytes, &token) == DVM_RETIRE_OK)
        {
            fvm_port_sync_lock(candidate_transfer);
            if (!cycle_candidate_already_queued(key))
            {
                queued = dvm_retirement_commit_key(cycle_candidates, &token, value, key, &ready);
            }
            fvm_port_sync_unlock(candidate_transfer);
            if (!queued && !dvm_retirement_cancel(cycle_candidates, &token))
            {
                abort();
            }
        }
        if (!queued)
        {
            DVM_RetirementResult result = dvm_retirement_reserve(retirement, bytes, &token);
            if (result != DVM_RETIRE_OK || !dvm_retirement_commit_ready(retirement, &token, value, &ready))
            {
                fprintf(stderr, "disvm: mandatory GC release publication failed (%d)\n", result);
                abort();
            }
        }
        if (ready && notify_ready)
        {
            notify_ready();
        }
    }
    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&helper_enqueued, helper != 0, __ATOMIC_RELAXED);
    return 1;
}
