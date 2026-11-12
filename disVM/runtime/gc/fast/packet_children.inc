/* Copyright (c) 2026 bhgv
 * EN: Bounded packet children and branch release.
 * RU: Ограниченное освобождение ссылок пакетов и ветвей.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Consume at most budget children of the oldest frame packet on the sole shared Fast consumer.
 * Unconsumed suffix retains its roots. Return completed child count; refusal preserves the next child
 * for retry. retry receives a pending cycle-publication retry.
 * RU: Снять до budget дочерних ссылок старейшего пакета на единственном shared Fast. Остаток сохраняет корни.
 * Вернуть число снятых ссылок; при отказе следующая ссылка остаётся для повтора. */
static unsigned int drain_shared_children(size_t budget, int *retry)
{
    unsigned int completed = 0;
    while (budget)
    {
        FrameRetirement *packet = dvm_retirement_peek(frames);
        if (!packet)
        {
            break;
        }
        while (budget && packet->next < packet->count)
        {
            Heap *heap = packet->keys[packet->next];
            int transferred = transfer_record_candidate(heap, packet, packet->next);
            if (!transferred && record_candidate(heap) && dvm_refcount_load_u(&heap->ref) > 1)
            {
                if (!preserve_packet_candidate(heap, packet, packet->next, retry))
                {
                    return completed;
                }
                transferred = 1;
            }
            if (!transferred)
            {
                if (dvm_refcount_drop_nonfinal_u(&heap->ref))
                {
                    dvm_runtime_gc_owner_lost();
                }
                else
                {
                    if (!leaf_eligible(heap) || !defer_leaf(heap, poolmsize(heapmem, heap), 0))
                    {
                        return completed;
                    }
                    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
                    __atomic_fetch_add(&helper_enqueued, 1, __ATOMIC_RELAXED);
                }
            }
            if (!transferred && !dvm_runtime_root_batch_remove(packet->roots, packet->next))
            {
                abort();
            }
            packet->next++;
            shared_children++;
            completed++;
            budget--;
        }
        if (packet->next == packet->count)
        {
            void *value;
            unsigned long bytes;
            if (!dvm_retirement_take(frames, &value, &bytes) || value != packet ||
                !dvm_runtime_root_batch_clear(packet->roots) || !dvm_runtime_root_batch_destroy(&packet->roots))
            {
                abort();
            }
            free(packet);
            frames_drained++;
        }
    }
    return completed;
}


/* EN: Root count owners from keys. Charge payload bytes for the first
 * charged_keys entries. token receives the reservation; retry reports temporary
 * memory or queue pressure. Invalid sizes/charges preserve inputs without retry.
 * RU: Удержать count ссылок из keys. Учесть память первых charged_keys объектов.
 * token получает резервацию; retry сообщает о нехватке памяти или места очереди.
 * Неверные размеры/учёт сохраняют входы без повтора. */
static FrameRetirement *prepare_release_packet(
    void *const *keys, size_t count, size_t charged_keys, DVM_RetirementReservation *token, int *retry)
{
    unsigned long charge;
    if (!count || charged_keys > count || count > (SIZE_MAX - sizeof(FrameRetirement)) / sizeof(void *))
    {
        return NULL;
    }
    size_t storage = sizeof(FrameRetirement) + count * sizeof(void *);
    if (!dvm_gc_packet_charge(keys, charged_keys, storage, ULONG_MAX, &charge))
    {
        return NULL;
    }
    DVM_RetirementResult reservation = dvm_retirement_reserve(frames, charge, token);
    if (reservation != DVM_RETIRE_OK)
    {
        if (retry)
        {
            *retry = reservation == DVM_RETIRE_FULL || reservation == DVM_RETIRE_BYTES;
        }
        return NULL;
    }
    FrameRetirement *packet = malloc(sizeof(*packet) + count * sizeof(void *));
    DVM_RootBatch *roots = dvm_runtime_root_batch_create(count);
    if (!packet || !roots)
    {
        if (retry)
        {
            *retry = 1;
        }
        if (roots)
        {
            dvm_runtime_root_batch_destroy(&roots);
        }
        free(packet);
        dvm_retirement_cancel(frames, token);
        return NULL;
    }
    packet->roots = roots;
    packet->count = count;
    packet->next = 0;
    memcpy(packet->keys, keys, count * sizeof(void *));
    if (!publish_release_roots(roots, packet->keys, count))
    {
        dvm_runtime_root_batch_destroy(&roots);
        free(packet);
        dvm_retirement_cancel(frames, token);
        return NULL;
    }
    return packet;
}


/* EN: Transfer one hold per live node and every dead-to-live edge in group.
 * Refusal preserves existing roots. Charge each live allocation only once.
 * RU: Передать удержания живых узлов и каждую ссылку на них из мёртвых узлов group.
 * При отказе корни сохраняются. Память каждого живого объекта учитывается один раз. */
static int release_record_branches(const RecordGroup *group)
{
    if (!group->live_count)
    {
        return 1;
    }
    void *local_keys[RECORD_NODE_CAPACITY + RECORD_EDGE_LIMIT];
    size_t capacity = (size_t)group->nodes_used + group->edges_used;
    void **keys = local_keys;
    if (capacity > sizeof(local_keys) / sizeof(*local_keys))
    {
        keys = malloc(capacity * sizeof(*keys));
        if (!keys)
        {
            return 0;
        }
    }
    size_t count = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        if (group->live[index])
        {
            keys[count++] = D2H(group->nodes[index]);
        }
    }
    size_t unique = count;
    for (unsigned int edge = 0; edge < group->edges_used; edge++)
    {
        unsigned int target = group->targets[edge];
        if (target != UINT_MAX && group->live[target])
        {
            keys[count++] = D2H(group->nodes[target]);
        }
    }
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(keys, count, unique, &token, NULL);
    if (keys != local_keys)
    {
        free(keys);
    }
    if (!packet)
    {
        return 0;
    }
    if (!dvm_retirement_commit(frames, &token, packet))
    {
        abort();
    }
    return 1;
}
