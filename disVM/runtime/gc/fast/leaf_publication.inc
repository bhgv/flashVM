/* Copyright (c) 2026 bhgv
 * EN: Rooted leaf publication, childless claims and descriptor owners.
 * RU: Публикация листьев с корнями, захват без детей и владение дескрипторами.
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
    return dvm_proxy_gc_eligible(heap) || dvm_gc_fast_file_eligible(heap) || dvm_gc_fast_table_eligible(heap) || dvm_gc_fast_module_eligible(heap) ||
           (heap->t &&
            (((dvm_runtime_idle_cycles || concurrent_only) && heap->t == &Tlist) || heap->t->free == freeheap));
}


/* [GC:] EN: Check independently owned heap before a final callback. In strict
 * mode unsupported callbacks terminate with roots/counts intact; no silent leak.
 * RU: Проверить собственный heap перед финальным callback. В строгом режиме
 * неподдержанный callback завершает процесс, сохраняя корни/ссылки без скрытой утечки. */
static void require_shared_finalizer(const Heap *heap)
{
    if (concurrent_only && !leaf_eligible(heap))
    {
        fprintf(stderr,
                "NATIVE_CONCURRENCY_ERROR component=Fast reason=unsupported-finalizer heap=%p type=%p\n",
                (const void *)heap,
                (const void *)heap->t);
        fflush(stderr);
        _Exit(70);
    }
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


/* [GC:] EN: Sample independently live heap before child transfer. Return one
 * only for a sole owner; preserve a competing-count refusal in retry without
 * changing references, roots or fields. Caller has passed its reader cutoff.
 * RU: Проверить живой heap перед передачей детей после эпохи читателей. Один —
 * единственный владелец; другой счётчик сохраняет отказ в retry без изменения
 * ссылок, корней или полей. */
static int leaf_has_sole_owner(Heap *heap, int *retry)
{
    int sole = dvm_refcount_load_u(&heap->ref) == 1;
    *retry |= !sole;
    return sole;
}


/* [GC:] EN: Claim heap's independently owned last reference without children.
 * Caller has passed its reader cutoff. Return one on claim; on failure preserve
 * ownership and set retry even if the competing owner disappears immediately.
 * RU: Захватить последнюю собственную ссылку heap без дочернего пакета после
 * эпохи читателей. Один — успех; отказ сохраняет владение и выставляет retry,
 * даже если конкурирующий владелец немедленно исчезает. */
static int claim_childless_leaf(Heap *heap, int *retry)
{
    int claimed = dvm_refcount_claim_last_u(&heap->ref);
    *retry |= !claimed;
    return claimed;
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
    /* [GC:] EN: Cycles also publishes sole owners here; shared admission does
     * not mean Fast is already running. Wake its lane on every ready transition.
     * RU: Cycles тоже передаёт сюда последних владельцев. Shared-допуск не
     * означает работу Fast; каждое появление готовой головы будит его поток. */
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}
