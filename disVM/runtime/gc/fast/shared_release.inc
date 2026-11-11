/* Copyright (c) 2026 bhgv
 * EN: Epoch-gated shared leaf and graph reclamation.
 * RU: Параллельное освобождение листьев и графов после эпохи.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */
static int retirement_ready(int include_shared_retries); /* Queue metadata only. */


/* [GC:] EN: Withdraw source head and its root for entry on the sole shared Fast consumer. Caller retains entry
 * metadata until release completes; mismatch aborts.
 * RU: Снять голову source и корень entry на единственном shared Fast. Вызывающий хранит метаданные entry до
 * конца освобождения; несовпадение вызывает abort. */
static void take_shared_leaf(DVM_RetirementQueue *source, LeafRetirement *entry)
{
    void *value;
    unsigned long bytes;
    if (!dvm_retirement_take(source, &value, &bytes) || value != entry || !dvm_runtime_root_remove(entry->heap))
    {
        abort();
    }
}


/* [GC:] EN: Run bounded shared Fast work on the bound context, with at most budget visits/claims per category. No
 * blocking admission or cycle discovery occurs here. Keep roots on allocation/claim refusal; return
 * progress or an epoch retry to avoid a stopped fallback while readers remain.
 * RU: Выполнить ограниченный shared Fast на привязанном context с бюджетом budget просмотров/захватов по
 * категориям. Здесь нет ожидания допуска или обхода циклов. При отказе выделения/захвата корни сохраняются;
 * вернуть работу или повтор эпохи вместо остановленного fallback при живых читателях. */
unsigned int dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    if (!dvm_retirement_peek(leaves) && !dvm_retirement_peek(frames) && !dvm_retirement_peek(record_retries) &&
        !dvm_retirement_peek(retirement))
    {
        return 0;
    }
    DVM_HeapOwner *reader = dvm_native_owner_current();
    if (context != dvm_native_context_current() || !dvm_native_owner_attach_service(reader, world))
    {
        return 0;
    }
    unsigned int completed = 0;
    int retry = 0;
    if (dvm_native_owner_try_enter_idle_shared(reader))
    {
        size_t owner_budget = budget / 3 + (budget % 3 != 0);
        if (!heapmonitor && dvm_retirement_peek(retirement))
        {
            completed += drain_shared_owners(owner_budget, &retry);
            budget -= owner_budget;
        }
        DVM_RetirementStats retry_stats = {0};
        (void)dvm_retirement_statistics(record_retries, &retry_stats);
        size_t retries = budget / 2 + budget % 2;
        if (retries > retry_stats.ready)
        {
            retries = retry_stats.ready;
        }
        while (budget && !heapmonitor)
        {
            DVM_RetirementQueue *source = leaves;
            if (retries)
            {
                retries--;
                source = record_retries;
            }
            LeafRetirement *entry = dvm_retirement_peek(source);
            if (!entry || !dvm_world_epoch_elapsed(world, entry->epoch))
            {
                if (entry)
                {
                    retry = 1;
                }
                if (source == record_retries)
                {
                    dvm_retirement_rotate(source);
                    continue;
                }
                if (entry && entry->records && move_leaf(source, record_retries, entry))
                {
                    budget--;
                    completed++;
                    continue;
                }
                break;
            }
            if (entry->records)
            {
                size_t cost = entry->records->count + entry->snapshot->edges_used;
                unsigned long final_epoch = 0;
                DVM_CellResult claim;
                if (entry->claimed)
                {
                    claim = DVM_CELL_ACQUIRED;
                }
                else if (entry->claim_refused)
                {
                    claim = DVM_CELL_REFUSED;
                }
                else if (entry->pinned_children && dvm_world_has_borrowers(world))
                {
                    claim = DVM_CELL_BUSY;
                }
                else
                {
                    int reserved;
                    claim = dvm_gc_fast_confirm_graph(
                        entry->snapshot, record_cells, world, entry->pinned_children ? &final_epoch : NULL, &reserved);
                    fast_reserved_confirmations += reserved;
                    if (claim == DVM_CELL_ACQUIRED)
                    {
                        fast_ignored_live_fields += entry->snapshot->omitted_live_fields;
                    }
                    budget = cost < budget ? budget - cost : 0;
                }
                if (claim != DVM_CELL_ACQUIRED)
                {
                    entry->claim_refused = claim != DVM_CELL_BUSY;
                    int retry = claim == DVM_CELL_BUSY;
                    if (entry->claim_refused)
                    {
                        DVM_RetirementResult release = release_refused_snapshot(source, entry);
                        if (release == DVM_RETIRE_OK)
                        {
                            budget -= budget != 0;
                            completed++;
                            continue;
                        }
                        retry = release == DVM_RETIRE_FULL || release == DVM_RETIRE_BYTES;
                    }
                    if (retry && source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    DVM_RetirementQueue *destination = retry ? record_retries : difficult;
                    if (!move_leaf(source, destination, entry))
                    {
                        if (source == record_retries)
                        {
                            continue;
                        }
                        break;
                    }
                    budget -= budget != 0;
                    completed++;
                    continue;
                }
                if (final_epoch)
                {
                    entry->claimed = 1;
                    entry->epoch = final_epoch;
                    budget -= budget != 0;
                    completed++;
                    if (source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    if (move_leaf(source, record_retries, entry))
                    {
                        continue;
                    }
                    break;
                }
                if (entry->claimed && !release_record_branches(entry->snapshot))
                {
                    if (source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    break;
                }
                void *value;
                unsigned long bytes;
                if (!dvm_retirement_take(source, &value, &bytes) || value != entry)
                {
                    abort();
                }
                shared_record_groups_with_peer += dvm_world_has_active_guest(world);
                shared_record_confirmations += entry->pinned_children;
                /* A claimed group must leave the traced heap as a unit. Charge
                 * the full group so another large reclamation waits its turn. */
                reclaim_detached_records(entry, 1);
                shared_record_groups++;
                free(entry);
                __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
                budget = cost < budget ? budget - cost : 0;
                completed++;
                continue;
            }
            if (!leaf_eligible(entry->heap))
            {
                break;
            }
            if (dvm_refcount_drop_nonfinal_u(&entry->heap->ref))
            {
                take_shared_leaf(source, entry);
                dvm_runtime_gc_owner_lost();
                free(entry);
                __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
                budget--;
                completed++;
                continue;
            }
            Type *mapped_channel = NULL;
            int channel = entry->heap->t == &Tchannel;
            Array *array = entry->heap->t == &Tarray ? H2D(Array *, entry->heap) : NULL;
            int has_children = array && (array->root != H || array->t->np);
            int record = entry->heap->t->free == freeheap;
            int list = entry->heap->t == &Tlist;
            int module = dvm_gc_fast_module_eligible(entry->heap);
            int retry_leaf = 0;
            if ((module && !transfer_module_data(entry->heap, &retry_leaf)) ||
                (channel && !claim_quiet_channel(entry->heap, &mapped_channel, &retry_leaf)) ||
                (has_children && !transfer_array_children(entry->heap, &retry_leaf)) ||
                ((record || list) && !transfer_object_fields(entry->heap, &retry_leaf)))
            {
                retry_leaf |= dvm_refcount_load_u(&entry->heap->ref) > 1;
                if (retry_leaf && source == record_retries)
                {
                    dvm_retirement_rotate(source);
                    continue;
                }
                if (!move_leaf(source, retry_leaf ? record_retries : difficult, entry))
                {
                    if (source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    break;
                }
                budget--;
                completed++;
                continue;
            }
            take_shared_leaf(source, entry);
            DVM_RefcountResult result = has_children || channel || record || list || module
                                            ? DVM_REFCOUNT_LAST
                                            : dvm_refcount_release_u(&entry->heap->ref);
            if (result == DVM_REFCOUNT_LAST)
            {
                Type *type = entry->heap->t;
                int peer = dvm_world_has_active_guest(world);
                if (type == &Tarray)
                {
                    release_owned_descriptor(H2D(Array *, entry->heap)->t);
                    shared_arrays++;
                    shared_array_graphs += has_children;
                    shared_arrays_with_peer += peer;
                }
                else if (channel)
                {
                    if (mapped_channel)
                    {
                        release_owned_descriptor(mapped_channel);
                    }
                    shared_channels++;
                }
                else if (list)
                {
                    Type *element = H2D(List *, entry->heap)->t;
                    if (element)
                    {
                        release_owned_descriptor(element);
                    }
                    shared_lists++;
                }
                else if (module)
                {
                    dvm_gc_fast_module_finalize(entry->heap);
                    shared_modules++;
                    shared_modules_with_peer += peer;
                }
                else if (dvm_gc_fast_file_eligible(entry->heap))
                {
                    dvm_gc_fast_file_finalize(entry->heap);
                    shared_files++;
                    shared_files_with_peer += peer;
                }
                else if (dvm_gc_fast_table_eligible(entry->heap))
                {
                    int kind = dvm_gc_fast_table_finalize(entry->heap);
                    shared_tables += kind == DVM_BIGTABLE_NATIVE_TABLE;
                    shared_keys += kind == DVM_BIGTABLE_NATIVE_KEYS;
                    shared_table_owners_with_peer += peer;
                }
                else if (!record)
                {
                    free(H2D(String *, entry->heap)->tmp);
                    shared_strings++;
                    shared_strings_with_peer += peer;
                }
                if (record)
                {
                    release_owned_descriptor(type);
                    shared_plain_records++;
                }
                else if (dvm_refcount_release_i(&type->ref) != DVM_REFCOUNT_LIVE)
                {
                    abort();
                }
                poolfree(heapmem, entry->heap);
            }
            else if (result != DVM_REFCOUNT_LIVE)
            {
                abort();
            }
            free(entry);
            __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
            budget--;
            completed++;
        }
        if (!heapmonitor)
        {
            completed += drain_shared_children(budget, &retry);
            /* An accepting retry queue is outside the exclusive drain.
             * Ask for another paced shared pass if nothing else can run. */
            DVM_RetirementStats pending = {0};
            (void)dvm_retirement_statistics(record_retries, &pending);
            retry |= !completed && pending.accepting && pending.ready && !retirement_ready(0) &&
                     !dvm_managed_owner_pending();
        }
        if (!dvm_native_owner_leave_idle_shared(reader))
        {
            abort();
        }
    }

    if (!dvm_native_owner_detach(reader))
    {
        abort();
    }
    return completed + retry;
}
