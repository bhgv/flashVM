/* Copyright (c) 2026 bhgv
 * EN: Preserve a packet's cycle seed when indexed publication cannot proceed.
 * RU: Сохранить seed цикла из пакета при отказе индексированной публикации.
 */


/* [GC:] EN: Transfer packet[index]'s existing strong heap owner to the mandatory
 * FIFO on the sole shared Fast consumer. Return 1 on transfer, 0 unchanged on
 * refusal. Publish a replacement individual root before removing the batch root;
 * never decrement a candidate merely because its indexed slot is unavailable.
 * Reserve native storage before publication; refusal leaves packet cursor, root
 * and reference intact. retry marks recoverable live-admission pressure, while
 * closed admission preserves the existing retained route. No payload access follows
 * commit; queues/world survive all joins. The caller advances the packet cursor.
 * RU: Передать существующую ссылку packet[index] на heap в обязательный FIFO
 * единственным shared Fast. Вернуть 1 при передаче, 0 без изменения при отказе.
 * Новый отдельный корень публикуется до снятия пакетного; не снимать кандидата
 * только из-за недоступного индекса. Резервировать память до публикации. Отказ
 * сохраняет курсор, корень и ссылку. retry отмечает временное заполнение при живом
 * допуске; закрытие сохраняет резервный путь. После commit данные не читаются;
 * мир/очереди живут до join. Курсор продвигает вызывающий.
 */
static int preserve_packet_candidate(Heap *heap, FrameRetirement *packet, size_t index, int *retry)
{
    if (!accepting)
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    unsigned long bytes = poolmsize(heapmem, heap);
    DVM_RetirementResult reserved = dvm_retirement_reserve(retirement, bytes, &token);
    if (reserved != DVM_RETIRE_OK)
    {
        *retry |= reserved == DVM_RETIRE_FULL || reserved == DVM_RETIRE_BYTES;
        packet_candidate_retries++;
        return 0;
    }
    if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop) || !dvm_runtime_root_add(heap))
    {
        if (!dvm_retirement_cancel(retirement, &token))
        {
            abort();
        }
        *retry = 1;
        packet_candidate_retries++;
        return 0;
    }
    int ready;
    if (!dvm_runtime_root_batch_remove(packet->roots, index) ||
        !dvm_retirement_commit_ready(retirement, &token, H2D(void *, heap), &ready))
    {
        abort();
    }
    packet_candidates_preserved++;
    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&helper_enqueued, 1, __ATOMIC_RELAXED);
    if (ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}
