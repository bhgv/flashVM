/* Copyright (c) 2026 bhgv
 * EN: Rooted leaf publication and descriptor owners.
 * RU: Публикация листьев с корнями и владение дескрипторами.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Publish count live heap headers in keys through roots, marking each first. Caller owns the references
 * and admission. Return false on a marking or batch-publication refusal without transferring the owners.
 * RU: Опубликовать count живых заголовков keys через roots, предварительно пометив каждый. Вызывающий владеет
 * ссылками и допуском. При отказе маркировки или пакета вернуть false без передачи ссылок. */
static int publish_release_roots(DVM_RootBatch *roots, void *const *keys, size_t count)
{
    for (size_t index = 0; index < count; index++)
    {
        Heap *heap = keys[index];
        if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop))
        {
            return 0;
        }
    }
    return dvm_runtime_root_batch_publish(roots, keys, count);
}


/* [GC:] EN: Read a heap kept live by the caller and identify supported shared destructors. Return boolean; arrays
 * with children require a separate transfer packet. No arbitrary free callback is authorized.
 * RU: Проверить живой heap вызывающего и распознать поддержанный shared-деструктор. Вернуть bool; детям
 * массива нужен отдельный пакет передачи. Произвольный free callback не разрешается. */
static int leaf_eligible(const Heap *heap)
{
    if (heap->t == &Tstring)
    {
        return 1;
    }
    if (heap->t == &Tchannel)
    {
        return H2D(const Channel *, heap)->buf != NULL;
    }
    if (heap->t == &Tarray)
    {
        const Array *array = H2D(const Array *, heap);
        return array->t && array->t != H;
    }
    return dvm_gc_fast_file_eligible(heap) || dvm_gc_fast_table_eligible(heap) || dvm_gc_fast_module_eligible(heap) ||
           (heap->t && ((dvm_runtime_idle_cycles && heap->t == &Tlist) || heap->t->free == freeheap));
}


/* [GC:] EN: Consume one independently owned type reference. Free descriptor storage only on LAST; corrupt counts
 * abort. No VM callback runs.
 * RU: Снять собственную ссылку type. Освободить дескриптор только при LAST; повреждённый счётчик вызывает
 * abort. VM callbacks не вызываются. */
static void release_owned_descriptor(Type *type)
{
    DVM_RefcountResult result = dvm_refcount_release_i(&type->ref);
    if (result == DVM_REFCOUNT_LAST)
    {
        free(type->initialize);
        free(type);
    }
    else if (result != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
}


/* [GC:] EN: Publish the existing heap owner in the shared leaf queue. bytes
 * charges its allocation; exclusive selects an ACTIVE producer versus shared
 * service publication. Refusal preserves the original owner and registry root.
 * RU: Передать существующую ссылку heap в очередь листьев. bytes учитывает
 * выделение; exclusive различает ACTIVE-производителя и shared-службу.
 * При отказе исходная ссылка и её корень остаются у вызывающего. */
static int defer_leaf(Heap *heap, unsigned long bytes, int exclusive)
{
    if (!leaves || heapmonitor || !leaf_eligible(heap))
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    if (dvm_retirement_reserve(leaves, bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    LeafRetirement *entry = malloc(sizeof(*entry));
    if (!entry)
    {
        dvm_retirement_cancel(leaves, &token);
        return 0;
    }
    entry->heap = heap;
    entry->bytes = bytes;
    entry->records = NULL;
    entry->snapshot = NULL;
    entry->claim_refused = 0;
    entry->pinned_children = 0;
    entry->claimed = 0;
    entry->epoch = dvm_world_retire_epoch(world);
    if (!entry->epoch || (!exclusive && !dvm_gc_field_mark(&heap->color, &mutator, &nprop)) ||
        !dvm_runtime_root_add(heap))
    {
        free(entry);
        dvm_retirement_cancel(leaves, &token);
        return 0;
    }
    if (exclusive)
    {
        Setmark(heap);
    }
    int became_ready;
    if (!dvm_retirement_commit_ready(leaves, &token, entry, &became_ready))
    {
        abort();
    }
    if (exclusive && became_ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}
