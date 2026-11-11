/* Copyright (c) 2026 bhgv
 * EN: Root-preserving handoff of mandatory owners to shared cycle discovery.
 * RU: Передача обязательных ссылок в параллельный обход циклов с сохранением корня.
 */


/* [GC:] EN: Transfer the mandatory FIFO head for independently owned heap to
 * Cycles under shared Fast admission. Return 1 on handoff, 0 unchanged on retry,
 * -1 unchanged when shutdown/invalid identity prevents future publication.
 * An existing indexed candidate is NOT proof that its queue owner can be dropped:
 * Cycles may already be consuming that reference. Keep this owner/root until the
 * index clears, then publish another seed. Reserve before withdrawing the source;
 * the same root occurrence spans both queues. Only Fast consumes retirement.
 * Queue/world identities survive all producer/collector joins. No payload access
 * follows publication, as the destination may immediately consume the owner.
 * RU: Передать голову обязательного FIFO для собственного heap в Cycles под
 * shared Fast. Вернуть 1 при передаче, 0 при повторе без изменения,
 * -1 без изменения при закрытии/неверной идентичности. Запись в индексе
 * НЕ доказывает возможность снятия ссылки: Cycles уже может её потреблять.
 * Сохранить ссылку/корень до очистки индекса, затем опубликовать новый seed.
 * Резервировать до снятия source; один корень переживает обе очереди. retirement
 * читает только Fast. Мир/очереди живут до join. После публикации данные heap
 * не читаются: получатель может сразу снять ссылку.
 */
static int rescue_cycle_owner(Heap *heap)
{
    if (!accepting || !candidate_transfer)
    {
        return -1;
    }
    DVM_HeapKey key = dvm_heap_pool_key(heapmem, heap);
    unsigned long bytes = poolmsize(heapmem, heap);
    DVM_RetirementReservation token = {0};
    if (!key.generation)
    {
        return -1;
    }
    DVM_RetirementResult reserved = dvm_retirement_reserve(cycle_candidates, bytes, &token);
    if (reserved != DVM_RETIRE_OK)
    {
        return reserved == DVM_RETIRE_FULL || reserved == DVM_RETIRE_BYTES ? 0 : -1;
    }
    fvm_port_sync_lock(candidate_transfer);
    if (cycle_candidate_already_queued(key))
    {
        fvm_port_sync_unlock(candidate_transfer);
        if (!dvm_retirement_cancel(cycle_candidates, &token))
        {
            abort();
        }
        mandatory_cycle_retries++;
        return 0;
    }
    void *value;
    unsigned long prior_bytes;
    int ready;
    if (!dvm_retirement_take(retirement, &value, &prior_bytes) || value != H2D(void *, heap) || prior_bytes != bytes ||
        !dvm_retirement_commit_key(cycle_candidates, &token, value, key, &ready))
    {
        abort();
    }
    fvm_port_sync_unlock(candidate_transfer);
    mandatory_cycle_transfers++;
    if (ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}
