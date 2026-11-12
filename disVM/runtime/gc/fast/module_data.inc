/* Copyright (c) 2026 bhgv
 * EN: Publish an ordinary module instance's MP before claiming its final owner.
 * RU: Опубликовать MP обычного экземпляра модуля до захвата последней ссылки.
 */


/* [GC:] EN: On shared Fast, move heap's MP owner to a rooted release packet.
 * Return 1 only after last-count claim and publication; retry marks transient
 * refusal. A changed count rolls back packet roots/reservation without touching
 * the source MP/reference. RC must remain accepting through subsequent unload;
 * closed admission keeps the existing retained route. Only Fast consumes queues.
 * Preliminary count loss and empty-MP claim loss preserve the original retry
 * observation even when a short-lived competitor ends before the next sample.
 * RU: На shared Fast передать MP объекта heap в пакет с корнем. Вернуть 1 после
 * захвата последней ссылки и публикации; retry отмечает временный отказ. Изменение
 * счётчика откатывает корни/резервацию без изменения MP/ссылки источника. RC должен
 * принимать ссылки до последующего unload; закрытие сохраняет резервный путь.
 * Очереди читает только Fast.
 * Предварительный отказ счётчика и отказ захвата пустого MP сохраняют исходный
 * retry, даже если кратковременный конкурент исчезает до следующего чтения.
 */
static int transfer_module_data(Heap *heap, int *retry)
{
    if (!accepting || !dvm_gc_fast_module_eligible(heap) || !leaf_has_sole_owner(heap, retry))
    {
        return 0;
    }
    Modlink *link = H2D(Modlink *, heap);
    if (link->MP == H)
    {
        return claim_childless_leaf(heap, retry);
    }
    void *key = D2H(link->MP);
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(&key, 1, 1, &token, retry);
    if (!packet)
    {
        DVM_RetirementStats pending = {0};
        (void)dvm_retirement_statistics(frames, &pending);
        *retry |= accepting && pending.accepting;
        return 0;
    }
    if (dvm_refcount_claim_last_u(&heap->ref))
    {
        link->MP = H;
        if (!dvm_retirement_commit(frames, &token, packet))
        {
            abort();
        }
        return 1;
    }
    /* The failed module claim has not transferred any MP reference. */
    if (!dvm_runtime_root_batch_clear(packet->roots) || !dvm_runtime_root_batch_destroy(&packet->roots) ||
        !dvm_retirement_cancel(frames, &token))
    {
        abort();
    }
    free(packet);
    *retry = 1;
    return 0;
}
