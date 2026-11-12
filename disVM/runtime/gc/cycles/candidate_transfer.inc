/* Copyright (c) 2026 bhgv
 * EN: Candidate queue and root handoff.
 * RU: Передача очереди и корня кандидата.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */
/* Guarded incoming fields keep discovered children alive. Live candidates
 * can give up the queue owner here. Potential garbage retains its root for
 * the existing final confirmation path. */


/* [GC:] EN: Withdraw the sole candidate consumer head from source and remove the queue root for independently live
 * heap. Mismatch is fatal. Caller decides whether to release or transfer the strong reference.
 * RU: Снять голову source единственным читателем кандидатов и корень живого heap. Несовпадение фатально.
 * Вызывающий решает, снять сильную ссылку или передать её. */
static void take_cycle_candidate(DVM_RetirementQueue *source, Heap *heap)
{
    void *head;
    unsigned long bytes;
    if (!dvm_retirement_take(source, &head, &bytes) || head != H2D(void *, heap) || !dvm_runtime_root_remove(heap))
    {
        abort();
    }
}


/* [GC:] EN: Under shared admission, consume source's borrowed head only with
 * an independent registered root and a nonfinal atomic drop. Exclude other
 * candidate consumers' final live checks. Return one on consumption, zero intact.
 * RU: Под shared-допуском снять заимствованную голову source лишь при другом
 * корне и нефинальном атомарном снятии ссылки. Исключить итоговые live-проверки
 * других читателей кандидатов. Один — потреблён, ноль — сохранён. */
static int take_independently_rooted_candidate(DVM_RetirementQueue *source, Heap *heap)
{
    fvm_port_sync_lock(candidate_transfer);
    int consumed = dvm_runtime_root_count(heap) > 1 && dvm_refcount_drop_nonfinal_u(&heap->ref);
    if (consumed)
    {
        take_cycle_candidate(source, heap);
    }
    fvm_port_sync_unlock(candidate_transfer);
    return consumed;
}


/* EN: Move candidate and its root from source to destination. Return zero if full.
 * RU: Передать candidate и его корень из source в destination. При отказе вернуть ноль. */
int dvm_rc_move_candidate(DVM_RetirementQueue *source, DVM_RetirementQueue *destination, void *candidate)
{
    Heap *heap = D2H(candidate);
    DVM_RetirementReservation token = {0};
    unsigned long bytes = poolmsize(heapmem, heap);
    DVM_HeapKey key = {0};
    int indexed = destination && (destination == duke_candidates || destination == cycle_refused);
    if (indexed)
    {
        key = dvm_heap_pool_key(heapmem, heap);
        if (!key.generation)
        {
            return 0;
        }
    }
    if (dvm_retirement_reserve(destination, bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    if (indexed)
    {
        fvm_port_sync_lock(candidate_transfer);
    }
    void *value;
    unsigned long prior_bytes;
    if (!dvm_retirement_take(source, &value, &prior_bytes) || value != H2D(void *, heap) || prior_bytes != bytes ||
        !dvm_retirement_commit_key(destination, &token, value, key, NULL))
    {
        abort();
    }
    if (indexed)
    {
        fvm_port_sync_unlock(candidate_transfer);
    }
    if (notify_ready)
    {
        notify_ready();
    }
    return 1;
}
