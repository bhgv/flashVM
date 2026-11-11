/* Copyright (c) 2026 bhgv
 * EN: Closed graph publication to Fast.
 * RU: Передача замкнутых графов сборщику Fast.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Unlink captured group after closure confirmation with stopped admission. Keep one owner per dead node
 * before clearing fields; release live targets normally. Failures after root publication are fatal.
 * RU: Разорвать group после подтверждения замкнутости при остановленном допуске. Перед очисткой полей оставить
 * владельца каждого мёртвого узла; живые цели освобождать обычным путём. Ошибки после публикации корней
 * фатальны. */
static void unlink_record_group(const RecordGroup *group)
{
    for (unsigned int index = 1; index < group->nodes_used; index++)
    {
        if (!group->live[index] && dvm_refcount_retain_u(&D2H(group->nodes[index])->ref) != DVM_REFCOUNT_LIVE)
        {
            abort();
        }
    }
    for (unsigned int index = 0; index < group->edges_used; index++)
    {
        if (group->targets[index] == UINT_MAX || (group->sources && group->live[group->sources[index]]))
        {
            continue;
        }
        void *edge = H;
        unsigned int target = group->targets[index];
        void *value = group->nodes[target];
        memcpy(group->cells[index], &edge, sizeof(edge));
        if (group->live[target])
        {
            destroy(value);
        }
        else if (!dvm_refcount_drop_nonfinal_u(&D2H(value)->ref))
        {
            abort();
        }
    }
}


/* EN: Publish group for Fast using guarded fields or existing Duke pins.
 * shared selects the parallel path. retry reports transient pressure.
 * RU: Передать group в Fast под защитой полей или с удержаниями ДюкGC.
 * shared выбирает параллельный путь. retry сообщает о временном отказе. */
static int defer_record_group(RecordGroup *group, int shared, int *retry)
{
    *retry = 0;
    unsigned int edges;
    if (!dvm_rc_snapshot_edges(group, &edges))
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    size_t packet_bytes = sizeof(FrameRetirement) + group->nodes_used * sizeof(void *);
    size_t metadata = sizeof(LeafRetirement) + dvm_rc_record_group_bytes(group, edges) + packet_bytes +
                      dvm_runtime_root_batch_bytes(group->nodes_used);
    if (metadata > ULONG_MAX)
    {
        return 0;
    }
    unsigned long bytes = metadata;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        unsigned long size = poolmsize(heapmem, D2H(group->nodes[index]));
        if (size > ULONG_MAX - bytes)
        {
            return 0;
        }
        bytes += size;
    }
    if (dvm_world_has_borrowers(world))
    {
        *retry = 1;
        return 0;
    }
    DVM_RetirementResult reserved = dvm_retirement_reserve(leaves, bytes, &token);
    if (reserved != DVM_RETIRE_OK)
    {
        *retry = reserved == DVM_RETIRE_FULL || reserved == DVM_RETIRE_BYTES;
        return 0;
    }
    LeafRetirement *entry = malloc(sizeof(*entry));
    FrameRetirement *records = malloc(packet_bytes);
    RecordGroup *snapshot = dvm_rc_copy_record_group(group, edges, H);
    DVM_RootBatch *roots = dvm_runtime_root_batch_create(group->nodes_used);
    unsigned long epoch = dvm_world_retire_epoch(world);
    if (!entry || !records || !snapshot || !roots || !epoch)
    {
        if (roots)
        {
            dvm_runtime_root_batch_destroy(&roots);
        }
        free(snapshot);
        free(records);
        free(entry);
        dvm_retirement_cancel(leaves, &token);
        return 0;
    }
    records->roots = roots;
    records->count = group->nodes_used;
    records->next = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        records->keys[index] = D2H(group->nodes[index]);
    }
    int pin_children = shared || group->live_count || group->has_views;
    unsigned int adopted = group->pinned_nodes;
    if (adopted && (!shared || adopted != group->nodes_used - 1))
    {
        abort();
    }
    if (pin_children)
    {
        for (unsigned int index = 1; index < group->nodes_used; index++)
        {
            if (!adopted && dvm_refcount_retain_u(&D2H(group->nodes[index])->ref) != DVM_REFCOUNT_LIVE)
            {
                abort();
            }
            snapshot->incoming[index]++;
        }
    }
    if (!dvm_runtime_root_batch_publish(roots, records->keys, records->count))
    {
        abort();
    }
    /* Publish every replacement root before withdrawing the discovery roots.
     * The snapshot now owns the same strong references without another retain. */
    while (group->pinned_nodes)
    {
        if (!dvm_runtime_root_remove(records->keys[group->pinned_nodes]))
        {
            abort();
        }
        group->pinned_nodes--;
    }
    for (size_t index = 0; index < records->count; index++)
    {
        Heap *heap = records->keys[index];
        if (shared)
        {
            if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop))
            {
                abort();
            }
        }
        else
        {
            Setmark(heap);
        }
    }
    *entry = (LeafRetirement){
        .bytes = bytes, .epoch = epoch, .records = records, .snapshot = snapshot, .pinned_children = pin_children};
    int became_ready;
    if (!dvm_retirement_commit_ready(leaves, &token, entry, &became_ready))
    {
        abort();
    }
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}
