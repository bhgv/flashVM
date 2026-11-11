/* Copyright (c) 2026 bhgv
 * EN: Detached graph reclamation and retry routing.
 * RU: Освобождение отделённых графов и маршруты повторов.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Reclaim the closed graph in entry after its required epoch. shared selects claimed zero-count nodes
 * versus stopped destruction. Caller owns the entry and roots; this consumes graph references, not entry
 * storage.
 * RU: Освободить замкнутый граф entry после нужной эпохи. shared различает захваченные нулевые счётчики и
 * остановленный путь. Вызывающий владеет entry и корнями; функция снимает ссылки графа, но не память entry. */
static void reclaim_detached_records(LeafRetirement *entry, int shared)
{
    FrameRetirement *records = entry->records;
    if (!shared && !entry->claim_refused && !entry->pinned_children)
    {
        unlink_record_group(entry->snapshot);
    }
    RecordGroup *snapshot = entry->snapshot;
    if (!dvm_runtime_root_batch_clear(records->roots) || !dvm_runtime_root_batch_destroy(&records->roots))
    {
        abort();
    }
    size_t owners = entry->claim_refused && !entry->pinned_children ? 1 : records->count;
    for (size_t index = 0; index < owners; index++)
    {
        Heap *heap = records->keys[index];
        if ((shared || entry->claimed) && snapshot->live[index])
        {
            if (!shared)
            {
                destroy(H2D(void *, heap));
            }
        }
        else if (shared || entry->claimed)
        {
            if (heap->t == &Tarray)
            {
                release_owned_descriptor(H2D(Array *, heap)->t);
            }
            if (heap->t == &Tchannel)
            {
                Type *mapped = NULL;
                if (!dvm_runtime_channel_claim_quiet(heap, H2D(Channel *, heap)->buf, &mapped, 1))
                {
                    abort();
                }
                if (mapped)
                {
                    release_owned_descriptor(mapped);
                }
                shared_channels += shared;
            }
            if (heap->t == &Tlist && H2D(List *, heap)->t)
            {
                release_owned_descriptor(H2D(List *, heap)->t);
            }
            release_owned_descriptor(heap->t);
            poolfree(heapmem, heap);
        }
        else
        {
            destroy(H2D(void *, heap));
        }
    }
    if (!shared && entry->claimed)
    {
        for (unsigned int edge = 0; edge < snapshot->edges_used; edge++)
        {
            unsigned int target = snapshot->targets[edge];
            if (target != UINT_MAX && snapshot->live[target])
            {
                destroy(snapshot->nodes[target]);
            }
        }
    }
    free(snapshot);
    free(records);
    record_groups += !entry->claim_refused && (shared || entry->claimed || !entry->pinned_children);
}


/* [GC:] EN: Move entry metadata from source to destination on the sole Fast consumer. Reserve before withdrawing
 * the head; refusal preserves reference/root and source. Return success without changing the managed
 * owner.
 * RU: Переместить метаданные entry из source в destination единственным Fast. Резервировать до снятия головы;
 * при отказе ссылка, корень и source сохраняются. Вернуть успех без смены управляемой ссылки. */
static int move_leaf(DVM_RetirementQueue *source, DVM_RetirementQueue *destination, LeafRetirement *entry)
{
    DVM_RetirementReservation token = {0};
    if (dvm_retirement_reserve(destination, entry->bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    void *value;
    unsigned long bytes;
    if (!dvm_retirement_take(source, &value, &bytes) || value != entry || bytes != entry->bytes ||
        !dvm_retirement_commit(destination, &token, entry))
    {
        abort();
    }
    difficult_moved += destination == difficult;
    return 1;
}


/* EN: Return strong references from a refused entry to Fast as a release
 * packet. source and entry remain intact if the packet cannot be queued.
 * RU: Передать сильные ссылки отклонённого entry в Fast. Если пакет нельзя
 * поставить в очередь, source и entry остаются целыми. */
static DVM_RetirementResult release_refused_snapshot(DVM_RetirementQueue *source, LeafRetirement *entry)
{
    FrameRetirement *records = entry->records;
    unsigned long charge =
        entry->bytes - sizeof(*entry) - dvm_rc_record_group_bytes(entry->snapshot, entry->snapshot->edges_used);
    DVM_RetirementReservation token = {0};
    DVM_RetirementResult result = dvm_retirement_reserve(frames, charge, &token);
    if (result != DVM_RETIRE_OK)
    {
        return result;
    }
    if (!entry->pinned_children)
    {
        /* Exclusive discovery published descendant roots without retaining
         * their counts. Only the initial node owns a releasable reference. */
        for (size_t index = 1; index < records->count; index++)
        {
            if (!dvm_runtime_root_batch_remove(records->roots, index))
            {
                abort();
            }
        }
        records->count = 1;
    }
    void *value;
    unsigned long bytes;
    if (!dvm_retirement_take(source, &value, &bytes) || value != entry || bytes != entry->bytes ||
        !dvm_retirement_commit(frames, &token, records))
    {
        abort();
    }
    free(entry->snapshot);
    free(entry);
    /* The accepted snapshot entry has completed its ownership stage. Packet
     * children enter accepted accounting only when they reach another queue;
     * discovered pins were never separate accepted entries before this replay. */
    __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
    snapshots_replayed++;
    return DVM_RETIRE_OK;
}
