/* Copyright (c) 2026 bhgv
 * EN: Legacy stopped fallback release, pending removal.
 * RU: Старое резервное освобождение при остановке, подлежащее замене.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Revalidate value under stopped collector admission before unlinking fields. promote permits
 * publication to Fast; return whether it transferred ownership. Retain a hold for every node through
 * unlinking; views require promotion.
 * RU: Повторно проверить value под остановленным допуском до разрыва полей. promote разрешает передачу в Fast;
 * вернуть факт передачи владения. При разрыве удерживать каждый узел; views требуют передачи. */
static int process_dvm_rc_queued_record_group(void *value, int promote)
{
    RecordGroupScratch scratch = {0};
    RecordGroup group = dvm_rc_empty_record_group(&scratch);
    if (!dvm_rc_queued_record_group(value, 1, &group, NULL) || group.live[0])
    {
        return 0;
    }
    int retry;
    if (promote && defer_record_group(&group, 0, &retry))
    {
        return 1;
    }
    /* Clearing a view's root would make its legacy destructor treat borrowed
     * data as owned. Leave unpromoted views to the retained tracer. */
    if (group.has_views)
    {
        return 0;
    }
    unlink_record_group(&group);
    for (unsigned int index = 1; index < group.nodes_used; index++)
    {
        if (!group.live[index])
        {
            destroy(group.nodes[index]);
        }
    }
    record_groups++;
    return 0;
}


/* [GC:] EN: Count the retiring value by type before stopped destruction can invalidate its header. Consume its
 * strong reference using the legacy destroy callback.
 * RU: Учесть тип value до остановленного разрушения, способного освободить заголовок. Снять сильную ссылку
 * прежним destroy callback. */
static void fallback_destroy(void *value)
{
    Heap *heap = D2H(value);
    Type *type = heap->t;
    if (dvm_refcount_load_u(&heap->ref) > 1)
    {
        fallback.nonfinal++;
    }
    else if (type == &Tarray)
    {
        fallback.arrays++;
    }
    else if (type == &Tlist)
    {
        fallback.lists++;
    }
    else if (type == &Tchannel)
    {
        fallback.channels++;
    }
    else if (type == &Tstring)
    {
        fallback.strings++;
    }
    else if (type == &Tmodlink)
    {
        fallback.modules++;
    }
    else if (type && type->free == freeheap)
    {
        fallback.records++;
    }
    else
    {
        fallback.other++;
    }
    destroy(value);
}


/* [GC:] Give a nonfinal fallback owner another shared discovery opportunity.
 * The caller has removed the FIFO entry but still owns its reference and root.
 * On success both move to Cycles. On refusal the caller releases them normally.
 * RU: Передать непоследнего владельца из резервной очереди обратно в Cycles.
 * Ссылка и корень остаются у вызывающего до успешной публикации. */
static int retry_retired_candidate(void *value, unsigned long bytes)
{
    Heap *heap = D2H(value);
    if (!accepting || !candidate_transfer || !record_candidate(heap) || dvm_refcount_load_u(&heap->ref) <= 1)
    {
        return 0;
    }
    DVM_HeapKey key = dvm_heap_pool_key(heapmem, heap);
    DVM_RetirementReservation token = {0};
    if (!key.generation || dvm_retirement_reserve(cycle_candidates, bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    fvm_port_sync_lock(candidate_transfer);
    int queued =
        !cycle_candidate_already_queued(key) && dvm_retirement_commit_key(cycle_candidates, &token, value, key, NULL);
    fvm_port_sync_unlock(candidate_transfer);
    if (!queued)
    {
        if (!dvm_retirement_cancel(cycle_candidates, &token))
        {
            abort();
        }
        return 0;
    }
    if (notify_ready)
    {
        notify_ready();
    }
    return 1;
}


/* [GC:] Release rooted queue owners only on a stopped-world collector. Each
 * destructor and its nested releases use the retained algorithm; they bypass
 * admission because the collector has no ACTIVE mutator ownership. Budget
 * bounds top-level owners, not the size of a recursively destroyed graph. */
unsigned int dvm_runtime_rc_drain(unsigned int budget)
{
    DVM_RetirementStats stats = {0};
    void *value;
    unsigned long bytes;
    if (!dvm_world_collector_owns(world))
    {
        fputs("disvm RC retirement outside stopped collector\n", stderr);
        abort();
    }
    fallback.passes++;
    if (!accepting && duke_inflight.owned_scratch)
    {
        if (duke_abandoned.owned_scratch)
        {
            abort();
        }
        duke_abandoned = duke_inflight;
        memset(&duke_inflight, 0, sizeof(duke_inflight));
        duke_publish_pending = 0;
    }
    if (budget && (!accepting || __atomic_load_n(&duke_cleanup_fallback, __ATOMIC_ACQUIRE)) &&
        duke_abandoned.owned_scratch)
    {
        size_t pins = duke_abandoned.pinned_nodes;
        (void)release_duke_pins(budget, 1);
        size_t consumed = pins - duke_abandoned.pinned_nodes;
        budget -= consumed;
    }
    while (budget &&
           ((!accepting && dvm_retirement_take(record_retries, &value, &bytes)) ||
            dvm_retirement_take(difficult, &value, &bytes) ||
            ((!dvm_runtime_idle_cycles || !accepting || heapmonitor) && dvm_retirement_take(leaves, &value, &bytes))))
    {
        LeafRetirement *entry = value;
        budget--;
        if (entry->records)
        {
            fallback.groups++;
            reclaim_detached_records(entry, 0);
        }
        else
        {
            ptrdel(entry->heap);
            fallback_destroy(H2D(void *, entry->heap));
        }
        free(entry);
        __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
    }
    /* During execution only individually refused candidates reach this pass.
     * The monitor and shutdown may drain the remaining shared queue. */
    int direct_release;
    while (budget && ((direct_release = dvm_retirement_take(retirement, &value, &bytes) ||
                                        (!accepting && dvm_retirement_take(duke_candidates, &value, &bytes))) ||
                      dvm_retirement_take(cycle_refused, &value, &bytes) ||
                      ((!accepting || heapmonitor) && dvm_retirement_take(cycle_candidates, &value, &bytes))))
    {
        budget--;
        if (direct_release && retry_retired_candidate(value, bytes))
        {
            continue;
        }
        /* Direct entries held the last owner or exhausted the snapshot
         * budget in Cycles. The retained tracer handles oversized closure. */
        int promoted = !direct_release && process_dvm_rc_queued_record_group(value, 1);
        ptrdel(D2H(value));
        fallback.promoted += promoted;
        if (!promoted)
        {
            fallback_destroy(value);
            __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
        }
    }
    /* Charge individual children so one frame cannot hold the idle heap
     * across all of its nested releases. Shared Fast can resume the suffix. */
    while (budget)
    {
        FrameRetirement *packet = dvm_retirement_peek(frames);
        if (!packet)
        {
            break;
        }
        while (budget && packet->next < packet->count)
        {
            size_t index = packet->next++;
            budget--;
            Heap *heap = packet->keys[index];
            void *child = H2D(void *, heap);
            int promoted = 0;
            int transferred = 0;
            if (dvm_runtime_idle_cycles && accepting && !heapmonitor)
            {
                if (dvm_refcount_load_u(&heap->ref) == 1 && defer_leaf(heap, poolmsize(heapmem, heap), 1))
                {
                    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
                    promoted = 1;
                }
                else
                {
                    transferred = transfer_record_candidate(heap, packet, index);
                    promoted = transferred;
                }
            }
            if (!promoted)
            {
                process_dvm_rc_queued_record_group(child, 0);
            }
            if (!transferred && !dvm_runtime_root_batch_remove(packet->roots, index))
            {
                abort();
            }
            if (promoted)
            {
                fallback.packet_promoted++;
            }
            else
            {
                fallback_destroy(child);
            }
        }
        if (packet->next != packet->count)
        {
            break;
        }
        if (!dvm_retirement_take(frames, &value, &bytes) || value != packet ||
            !dvm_runtime_root_batch_clear(packet->roots) || !dvm_runtime_root_batch_destroy(&packet->roots))
        {
            abort();
        }
        free(packet);
        frames_drained++;
    }
    DVM_RetirementStats frame_stats = {0};
    (void)dvm_retirement_statistics(frames, &frame_stats);
    (void)dvm_retirement_statistics(retirement, &stats);
    return stats.ready + stats.reserved + frame_stats.ready + frame_stats.reserved +
           (dvm_retirement_peek(leaves) != NULL) + (dvm_retirement_peek(difficult) != NULL) +
           (dvm_retirement_peek(record_retries) != NULL) + (dvm_retirement_peek(cycle_refused) != NULL) +
           dvm_runtime_rc_candidates_ready() +
           (!accepting && (dvm_runtime_rc_duke_ready() || duke_abandoned.pinned_nodes)) +
           __atomic_load_n(&duke_cleanup_fallback, __ATOMIC_ACQUIRE);
}
