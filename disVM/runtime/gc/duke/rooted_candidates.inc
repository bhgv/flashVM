/* Copyright (c) 2026 bhgv
 * EN: Bounded removal of independently rooted Duke queue owners during discovery.
 * RU: Ограниченное снятие владельцев очереди Duke с независимыми корнями при обходе.
 */

#define DUKE_ROOT_SWEEP_LIMIT 4096 /* Maximum FIFO entries inspected per discovery slice. */


/* [GC:] EN: With shared admission and the sole Duke consumer, preserve seed at
 * the head and remove other queue owners only when an independent root exists.
 * Discovery pins remain owned by the in-flight capture; their normal release or
 * rescue protocol still applies. Never infer ownership from an indexed key.
 * The transfer mutex excludes appenders while restoring FIFO order. Oversized
 * queues retain the ordinary one-candidate path; no allocation or final release.
 * Return the number of removed owners; the caller must invalidate captured
 * root observations before classification whenever this result is nonzero.
 * RU: Под shared-допуском единственным читателем Duke сохранить seed в голове
 * и снять прочих владельцев очереди лишь при независимом корне. Удержания обхода
 * остаются у снимка и освобождаются либо передаются обычным протоколом. Наличие
 * ключа не доказывает владение. Мьютекс передачи исключает публикацию на время
 * восстановления FIFO. Для слишком большой очереди оставить обычный путь;
 * выделения памяти и финального освобождения здесь нет. Вернуть число снятых
 * владельцев; при ненулевом результате вызывающий обязан признать прежние
 * наблюдения корней недействительными до классификации. */
static unsigned int prune_rooted_duke_candidates(void *seed)
{
    unsigned int removed = 0;
    fvm_port_sync_lock(candidate_transfer);
    DVM_RetirementStats queued = {0};
    (void)dvm_retirement_statistics(duke_candidates, &queued);
    if (queued.ready > 1 && queued.ready <= DUKE_ROOT_SWEEP_LIMIT)
    {
        dvm_retirement_rotate(duke_candidates);
        for (unsigned int index = 1; index < queued.ready; index++)
        {
            void *value = dvm_retirement_peek(duke_candidates);
            if (!value || value == seed)
            {
                abort();
            }
            Heap *heap = D2H(value);
            if (dvm_runtime_root_count(heap) > 1 && dvm_refcount_drop_nonfinal_u(&heap->ref))
            {
                take_cycle_candidate(duke_candidates, heap);
                dvm_runtime_gc_owner_lost();
                __atomic_fetch_add(&cycle_candidates_rooted, 1, __ATOMIC_RELAXED);
                __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
                removed++;
            }
            else
            {
                dvm_retirement_rotate(duke_candidates);
            }
        }
        if (dvm_retirement_peek(duke_candidates) != seed)
        {
            abort();
        }
    }
    fvm_port_sync_unlock(candidate_transfer);
    return removed;
}
