/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real source self-cycle, detached ring and large array of string leaves.
 * RU: Настоящий self-цикл source, отделяемое кольцо и большой массив строк.
 */


/* [GC:] EN: Create actual typed managed objects with real field ownership.
 * Duke must abandon stale discovery after detachment, free the separate ring
 * while source stays independently rooted, then free all remaining targets after
 * that owner is released. frame remains production-owned; Type creators stay live.
 * RU: Создать типизированные объекты с настоящим владением полями. Duke должен
 * отменить устаревший обход, освободить отдельное кольцо при живом source, затем
 * остаток после снятия внешнего владельца. frame принадлежит runtime; Type живут. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    uchar source_map = 0xe0;
    uchar ring_map = 0x80;
    Type *source_type = dtype(freeheap, 3 * sizeof(void *), &source_map, 1);
    Type *ring_type = dtype(freeheap, sizeof(void *), &ring_map, 1);
    require(source_type && ring_type, "actual managed source/ring descriptors");
    void *source = H2D(void *, heapz(source_type));
    void *ring[BranchNodes];
    for (unsigned index = 0; index < BranchNodes; index++)
    {
        ring[index] = H2D(void *, heapz(ring_type));
    }
    for (unsigned index = 0; index < BranchNodes; index++)
    {
        void *next = ring[(index + 1) % BranchNodes];
        require(dvm_refcount_retain_u(&D2H(next)->ref) == DVM_REFCOUNT_LIVE, "actual ring edge owner");
        memcpy(ring[index], &next, sizeof(next));
        Setmark(D2H(next));
    }
    for (unsigned index = 1; index < BranchNodes; index++)
    {
        require(dvm_refcount_drop_nonfinal_u(&D2H(ring[index])->ref), "consume private non-entry creator");
    }
    Heap *array_heap = nheap(sizeof(Array) + Leaves * sizeof(void *));
    require(dvm_refcount_retain_i(&Tarray.ref) == DVM_REFCOUNT_LIVE &&
                dvm_refcount_retain_i(&Tptr.ref) == DVM_REFCOUNT_LIVE,
            "actual array descriptor owners");
    array_heap->t = &Tarray;
    Array *array = H2D(Array *, array_heap);
    array->len = Leaves;
    array->t = &Tptr;
    array->root = H;
    array->data = (uchar *)(array + 1);
    for (unsigned index = 0; index < Leaves; index++)
    {
        String *leaf = newstring(8);
        memcpy(array->data + index * sizeof(void *), &leaf, sizeof(leaf));
        Setmark(D2H(leaf));
    }
    require(dvm_refcount_retain_u(&D2H(source)->ref) == DVM_REFCOUNT_LIVE, "actual source self edge");
    ((void **)source)[0] = source;
    ((void **)source)[1] = ring[0];
    ((void **)source)[2] = array;
    Setmark(D2H(source));
    Setmark(array_heap);
    pthread_mutex_lock(&sync);
    nodes[0] = D2H(source);
    for (unsigned index = 0; index < BranchNodes; index++)
    {
        nodes[index + 1] = D2H(ring[index]);
    }
    nodes[ArrayIndex] = array_heap;
    for (unsigned index = 0; index < Leaves; index++)
    {
        void *leaf;
        memcpy(&leaf, array->data + index * sizeof(void *), sizeof(leaf));
        nodes[ArrayIndex + 1 + index] = D2H(leaf);
    }
    node_count = TotalTargets;
    for (unsigned index = 0; index < node_count; index++)
    {
        keys[index] = dvm_heap_pool_key(heapmem, nodes[index]);
        require(keys[index].generation != 0, "real allocation generation");
    }
    pthread_mutex_unlock(&sync);
    DVM_OwnerTransfer ticket = {0};
    require(dvm_managed_owner_capture(source, &ticket) && dvm_refcount_drop_nonfinal_u(&D2H(source)->ref),
            "ticket consumes private source creator owner");
    require(dvm_owner_transfer_cancel(&ticket), "release real source ticket");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent native roots published");
    unsigned long long deadline = milliseconds() + 5000;
    int success = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        int drop_outside = outside_held && detached_freed == BranchNodes && dvm_runtime_root_count(nodes[0]) == 1 &&
                           dvm_runtime_root_count(array_heap) == 0 && dvm_refcount_load_u(&array_heap->ref) == 1;
        success = freed == node_count && !expired;
        pthread_mutex_unlock(&sync);
        if (drop_outside)
        {
            dvm_runtime_gc_native_enter(native);
            require(!destroyed[0] && !destroyed[ArrayIndex] && ((void **)source)[1] == H &&
                        ((void **)source)[2] == array && array->len == Leaves,
                    "outside source preserves detached fields and full array");
            pthread_mutex_lock(&sync);
            outside_held = 0;
            pthread_mutex_unlock(&sync);
            require(dvm_runtime_root_remove(nodes[0]), "consume only independent native source root");
            destroy(source);
            native = dvm_runtime_gc_native_try_leave();
            require(native > 0, "parent roots after outside source release");
        }
        if (success)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    if (!success)
    {
        pthread_mutex_lock(&sync);
        fprintf(
            stderr,
            "Duke pin state freed=%u/%u detached=%u outside=%d mutated=%d seed=%d refused=%d commits=%u pressure=%d passes=%u\n",
            freed,
            node_count,
            detached_freed,
            outside_held,
            mutated,
            seed_drained,
            refused,
            commits,
            slot_refused,
            slot_passes);
        pthread_mutex_unlock(&sync);
    }
    require(detached_freed == BranchNodes, "detached cycle lost its Duke candidate pin");
    require(success, "Duke pin retirement still requires heap quiescence");
    require(refused && mutated && seed_drained && commits, "actual stale discovery and preserved pins observed");
    require(SCENARIO != 1 || (slot_refused && slot_passes >= 8), "mandatory pressure retry observed");
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_i(&source_type->ref) == 1 && dvm_refcount_load_i(&ring_type->ref) == 1,
            "all physical source/ring Type owners retired");
    freetype(source_type);
    freetype(ring_type);
    pthread_mutex_lock(&sync);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fprintf(stderr,
            "PASS Duke pin retirement detached=%u targets=%u commits=%u pressure=%d passes=%u\n",
            detached_freed,
            node_count,
            commits,
            slot_refused,
            slot_passes);
}
