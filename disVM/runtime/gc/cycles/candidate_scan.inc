/* Copyright (c) 2026 bhgv
 * EN: Shared candidate discovery and graph publication.
 * RU: Параллельный обход кандидатов и публикация графа.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* EN: Send the current Cycles candidate to the next collector after a hard refusal.
 * RU: Передать текущего кандидата Cycles следующему сборщику после отказа. */
static int route_refused_cycle(void)
{
    void *value = dvm_retirement_peek(cycle_candidates);
    if (duke_candidates && dvm_runtime_idle_cycles && !gchalt && !heapmonitor)
    {
        DVM_HeapKey key = dvm_heap_pool_key(heapmem, D2H(value));
        if (dvm_retirement_contains_key(duke_candidates, key))
        {
            return 0;
        }
        return dvm_rc_move_candidate(cycle_candidates, duke_candidates, value);
    }
    return cycle_refused && dvm_rc_move_candidate(cycle_candidates, cycle_refused, value);
}


/* EN: Try the source head under shared admission. cost counts work, busy signals retry.
 * RU: Проверить первый элемент source под параллельным допуском. cost учитывает работу, busy означает повтор. */
int dvm_rc_scan_candidate(DVM_RetirementQueue *source, size_t *cost, int *busy)
{
    *cost = 1;
    *busy = 0;
    if (source == duke_candidates && duke_abandoned.pinned_nodes && !release_duke_pins(64, 0))
    {
        *busy = 1;
        return 0;
    }
    void *value = dvm_retirement_peek(source);
    if (!dvm_runtime_idle_cycles || !value)
    {
        return 0;
    }
    Heap *heap = D2H(value);
    int resume = source == duke_candidates && duke_inflight.owned_scratch;
    if (!resume && dvm_refcount_load_u(&heap->ref) == 1)
    {
        if (!defer_leaf(heap, poolmsize(heapmem, heap), 0))
        {
            *busy = !gchalt && !heapmonitor;
            return 0;
        }
        take_cycle_candidate(source, heap);
        return 1;
    }
    if (!resume && dvm_runtime_root_count(heap) > 1 && dvm_refcount_drop_nonfinal_u(&heap->ref))
    {
        take_cycle_candidate(source, heap);
        dvm_runtime_gc_owner_lost();
        __atomic_fetch_add(&cycle_candidates_rooted, 1, __ATOMIC_RELAXED);
        __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
        return 1;
    }
    RecordGroupScratch scratch = {0};
    RecordGroup group = dvm_rc_empty_record_group(&scratch);
    group.growing = source == duke_candidates;
    if (resume)
    {
        if (duke_inflight.nodes[0] != value)
        {
            fputs("disvm Duke candidate changed between slices\n", stderr);
            abort();
        }
        group = duke_inflight;
    }
    else if (group.growing && !dvm_rc_grow_record_group(&group, DUKE_INITIAL_FIELDS))
    {
        return 0;
    }
    DVM_RangeAccess access = {0};
    int publishing = resume && duke_publish_pending;
    int scanned;
    if (publishing)
    {
        scanned = !gchalt && !heapmonitor;
    }
    else
    {
        if (dvm_range_access_begin(record_cells, NULL, 0, NULL, 0, &access) != DVM_CELL_ACQUIRED)
        {
            if (resume)
            {
                return DVM_CANDIDATE_RESUME;
            }
            free(group.owned_scratch);
            *busy = 1;
            return 0;
        }
        unsigned int previous_fields = group.edges_used;
        scanned = group.growing ? dvm_rc_duke_capture_slice(value, &group, &access)
                                : dvm_rc_queued_record_group(value, 1, &group, &access);
        *cost += group.edges_used - previous_fields;
        if (scanned == DVM_DUKE_CAPTURE_RESUME)
        {
            if (!dvm_range_access_end(&access))
            {
                abort();
            }
            duke_inflight = group;
            duke_resumed_slices++;
            return DVM_CANDIDATE_RESUME;
        }
    }
    if (resume)
    {
        memset(&duke_inflight, 0, sizeof(duke_inflight));
        duke_publish_pending = 0;
    }
    unsigned int discovery_pins = group.pinned_nodes;
    int retry_promotion = 0;
    int promoted = scanned && !group.live[0] && defer_record_group(&group, 1, &retry_promotion);
    if (source == duke_candidates)
    {
        duke_promoted += promoted;
        if (promoted)
        {
            duke_adopted_pins += discovery_pins;
        }
        if (promoted && group.edges_used > duke_peak_fields)
        {
            duke_peak_fields = group.edges_used;
        }
    }
    __atomic_fetch_add(&shared_record_snapshots_with_peer,
                       !publishing && scanned && dvm_world_has_active_guest(world),
                       __ATOMIC_RELAXED);
    if (access.domain && !dvm_range_access_end(&access))
    {
        abort();
    }
    __atomic_fetch_add(&shared_record_scans, !publishing && scanned, __ATOMIC_RELAXED);
    if (group.growing && retry_promotion)
    {
        /* Fast will revalidate the fields. Keep the rooted snapshot intact
         * while its destination is temporarily unavailable. */
        duke_inflight = group;
        duke_publish_pending = 1;
        duke_publish_retries++;
        return DVM_CANDIDATE_RESUME;
    }
    *busy = group.busy || retry_promotion;
    int live = scanned && group.live[0];
    if (group.pinned_nodes)
    {
        duke_abandoned = group;
        if (!release_duke_pins(64, 0))
        {
            *busy = 1;
            return 0;
        }
    }
    else
    {
        free(group.owned_scratch);
    }
    if (group.oversized)
    {
        DVM_RetirementQueue *destination =
            source == cycle_candidates && duke_candidates &&
                    !dvm_retirement_contains_key(duke_candidates, dvm_heap_pool_key(heapmem, heap))
                ? duke_candidates
                : retirement;
        int moved = dvm_rc_move_candidate(source, destination, value);
        if (!moved && destination == duke_candidates)
        {
            *busy = 1;
        }
        if (source == cycle_candidates)
        {
            cycle_candidates_oversized += moved;
        }
        return moved;
    }
    if (!promoted && (!live || !dvm_refcount_drop_nonfinal_u(&D2H(value)->ref)))
    {
        return 0;
    }
    take_cycle_candidate(source, heap);
    if (!promoted)
    {
        dvm_runtime_gc_owner_lost();
        __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
    }
    return 1;
}
