/* Copyright (c) 2026 bhgv
 * EN: Indexed candidate identity and ownership transfer.
 * RU: Идентичность кандидатов и передача владения.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Inspect caller-owned heap for a supported cycle seed only when cycle discovery is enabled. Return
 * boolean; no ownership transfer or graph walk occurs.
 * RU: Проверить собственный heap как начало поддержанного цикла только при включённом обходе. Вернуть bool без
 * передачи владения или обхода графа. */
static int record_candidate(const Heap *heap)
{
    if (!dvm_runtime_idle_cycles || !heap->t || gchalt || heapmonitor)
    {
        return 0;
    }
    if (heap->t == &Tmodlink)
    {
        return dvm_gc_fast_module_eligible(heap);
    }
    if (heap->t == &Tarray)
    {
        const Array *array = H2D(const Array *, heap);
        return array->root && array->t && (array->root != H || (array->len > 0 && array->t->np));
    }
    return heap->t == &Tlist || heap->t == &Tchannel || (heap->t->free == freeheap && heap->t->np);
}


/* [GC:] Dis owners, Fast and retained release processing publish candidates.
 * The caller's reference keeps the allocation alive during this observation.
 * Publication rechecks all indexed queues under candidate_transfer.
 * RU: Ссылку при проверке удерживает вызывающий. Перед публикацией запись
 * повторно ищется во всех индексированных очередях под candidate_transfer. */
static int cycle_candidate_already_queued(DVM_HeapKey key)
{
    int queued = dvm_retirement_contains_key(cycle_candidates, key) ||
                 dvm_retirement_contains_key(duke_candidates, key) || dvm_retirement_contains_key(cycle_refused, key);
    __atomic_fetch_add(&cycle_candidates_coalesced, queued, __ATOMIC_RELAXED);
    return queued;
}


/* [GC:] EN: Transfer the packet owner at index to an indexed candidate queue. heap and packet stay live until
 * success. Preserve the packet reference/root on refusal; on success caller advances its packet cursor.
 * RU: Передать ссылку packet[index] в индексированную очередь кандидатов. heap и packet живут до успеха. При
 * отказе ссылка и корень пакета сохраняются; при успехе вызывающий продвигает курсор. */
static int transfer_record_candidate(Heap *heap, FrameRetirement *packet, size_t index)
{
    if (!candidate_transfer || !record_candidate(heap) || dvm_refcount_load_u(&heap->ref) <= 1)
    {
        return 0;
    }
    DVM_HeapKey key = dvm_heap_pool_key(heapmem, heap);
    if (!key.generation || cycle_candidate_already_queued(key))
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    unsigned long bytes = poolmsize(heapmem, heap);
    DVM_RetirementResult reservation = dvm_retirement_reserve(cycle_candidates, bytes, &token);
    if (reservation != DVM_RETIRE_OK)
    {
        cycle_candidates_full += reservation == DVM_RETIRE_FULL;
        return 0;
    }
    fvm_port_sync_lock(candidate_transfer);
    if (cycle_candidate_already_queued(key) || !dvm_gc_field_mark(&heap->color, &mutator, &nprop) ||
        !dvm_runtime_root_add(heap))
    {
        fvm_port_sync_unlock(candidate_transfer);
        dvm_retirement_cancel(cycle_candidates, &token);
        return 0;
    }
    int became_ready;
    if (!dvm_runtime_root_batch_remove(packet->roots, index) ||
        !dvm_retirement_commit_key(cycle_candidates, &token, H2D(void *, heap), key, &became_ready))
    {
        abort();
    }
    fvm_port_sync_unlock(candidate_transfer);
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&helper_enqueued, 1, __ATOMIC_RELAXED);
    return 1;
}
