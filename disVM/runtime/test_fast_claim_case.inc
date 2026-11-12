/* Copyright (c) 2026 bhgv
 * [GC:] EN: Build actual rings with zero, sparse and hundreds of native Modules.
 * RU: Создать настоящие кольца без Module, с одним и с сотнями native-Module.
 */


/* [GC:] EN: Own private construction until one real seed ticket is published.
 * frame stays production-owned. Module original MPs and instance MPs share real
 * strong owners; each native Module has an independent registry identity.
 * Require real physical frees after allocation recovery and final reader epochs.
 * RU: Владеть построением до публикации настоящего seed-ticket. frame принадлежит
 * runtime. Исходные MP и MP экземпляров имеют настоящих сильных владельцев;
 * каждый Module имеет отдельную запись реестра. Требовать физические free после
 * восстановления памяти и завершения последних эпох читателей. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    Type *retained_type = NULL;
    if (MixedPending)
    {
        retained_type = dtype(retained_destructor, sizeof(void *), NULL, 0);
        require(retained_type != NULL, "actual unsupported Type creator");
        Heap *heap = heapz(retained_type);
        pthread_mutex_lock(&sync);
        retained_target = heap;
        retained_key = dvm_heap_pool_key(heapmem, heap);
        pthread_mutex_unlock(&sync);
        require(dvm_runtime_rc_defer(H2D(void *, heap)), "publish actual unsupported owner to mandatory FIFO");
        require(dvm_runtime_root_count(heap) == 1, "actual retained backlog is rooted");
    }
    unsigned count = SCENARIO == 0 ? 0 : SCENARIO == 1 ? 1 : SCENARIO == 2 ? 513 : SCENARIO == 3 ? 514 : 520;
    uchar map = ReplayPressure ? 0xc0 : 0x80;
    Type *type = dtype(freeheap, (ReplayPressure ? 2 : 1) * sizeof(void *), &map, 1);
    require(type != NULL, "actual graph Type creator");
    void *values[NodeCount];
    Runtab empty[1] = {{0}};
    Import imports[1] = {{0}};
    for (unsigned index = 0; index < count; index++)
    {
        char name[96];
        snprintf(name, sizeof(name), "$FlashVM-native-claim-storage-%u", index);
        Module *module = builtinmod(name, empty, 0);
        require(module && dvm_refcount_retain_i(&type->ref) == DVM_REFCOUNT_LIVE, "actual Module Type owner");
        module->type = mallocz(sizeof(Type *), 1);
        require(module->type != NULL, "actual Module Type vector");
        module->type[0] = type;
        module->ntype = 1;
        module->origmp = H2D(uchar *, heapz(type));
        Modlink *link = linkmod(module, imports, 0);
        require(link != H && link->MP == H && dvm_refcount_retain_u(&D2H(module->origmp)->ref) == DVM_REFCOUNT_LIVE,
                "instance acquires actual original MP owner");
        link->MP = module->origmp;
        Setmark(D2H(link->MP));
        values[2 * index] = link;
        values[2 * index + 1] = link->MP;
        native_modules[index] = module;
    }
    for (unsigned index = 2 * count; index < NodeCount; index++)
    {
        values[index] = H2D(void *, heapz(type));
    }
    for (unsigned index = 0; index < NodeCount; index++)
    {
        if (index < 2 * count && !(index & 1))
        {
            continue;
        }
        void *next = values[(index + 1) % NodeCount];
        require(dvm_refcount_retain_u(&D2H(next)->ref) == DVM_REFCOUNT_LIVE, "actual ring field owner");
        memcpy(values[index], &next, sizeof(next));
        Setmark(D2H(next));
    }
    for (unsigned index = 1; index < NodeCount; index++)
    {
        if (index < 2 * count && (index & 1))
        {
            continue;
        }
        require(dvm_refcount_drop_nonfinal_u(&D2H(values[index])->ref), "consume private nonseed creator");
    }
    pthread_mutex_lock(&sync);
    for (unsigned index = 0; index < NodeCount; index++)
    {
        nodes[index] = D2H(values[index]);
        keys[index] = dvm_heap_pool_key(heapmem, nodes[index]);
        require(keys[index].generation, "real graph allocation generation");
    }
    __atomic_store_n(&module_count, count, __ATOMIC_RELEASE);
    node_count = NodeCount;
    pthread_mutex_unlock(&sync);
    DVM_OwnerTransfer ticket = {0};
    require(dvm_managed_owner_capture(values[0], &ticket) && dvm_refcount_drop_nonfinal_u(&D2H(values[0])->ref),
            "real seed ticket consumes creator");
    require(dvm_owner_transfer_cancel(&ticket), "retire real native seed ticket");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent roots published before collection");
    unsigned long long deadline = milliseconds() + 10000;
    int success = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        if (QuiescentAssist && !released && refusals >= 8)
        {
            epochs_held = 0;
            released = 1;
            pthread_cond_broadcast(&changed);
        }
        if (confirmed && epochs_held && held_passes >= 8)
        {
            require(!freed && !native_freed, "all physical frees wait for final reader epoch");
            epochs_held = 0;
        }
        success = confirmed && freed == NodeCount && native_freed == count && !expired;
        pthread_mutex_unlock(&sync);
        if (success)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    if (!success)
    {
        pthread_mutex_lock(&sync);
        DVM_GCWorkersState lanes;
        require(dvm_gc_workers_state(dvm_gc_runtime.workers, &lanes), "worker diagnostics available");
        unsigned long roots = 0;
        unsigned long refs = 0;
        for (unsigned index = 0; index < NodeCount; index++)
        {
            if (!destroyed[index])
            {
                roots += dvm_runtime_root_count(nodes[index]);
                refs += dvm_refcount_load_u(&nodes[index]->ref);
            }
        }
        fprintf(
            stderr,
            "mixed state freed=%u native=%u confirmed=%d held=%d/%u replay=%d/%d refusals=%u passes=%u refs=%lu roots=%lu lanes=%zu/%zu/%zu ready=%d/%d/%d\n",
            freed,
            native_freed,
            confirmed,
            epochs_held,
            held_passes,
            replay_stale,
            replay_done,
            replay_refusals,
            replay_passes,
            refs,
            roots,
            lanes.Fast.passes,
            lanes.Cycles.passes,
            lanes.Duke.passes,
            dvm_runtime_rc_ready(),
            dvm_runtime_rc_candidates_ready(),
            dvm_runtime_rc_duke_ready());
        pthread_mutex_unlock(&sync);
    }
    require(!QuiescentAssist || retained_calls, "quiescent retained work must not starve behind shared retry");
    require(success, "native claim storage still requires heap quiescence");
    require(count <= RECORD_NODE_CAPACITY ? !allocations && !refusals && !vector_frees
                                          : (QuiescentAssist ? refusals >= 8 && allocations == refusals + 1
                                                             : allocations == 9 && refusals == 8) &&
                                                vector_frees == 1,
            "actual vector allocation and recovery evidence");
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_i(&type->ref) == 1, "all managed and native Type owners released");
    freetype(type);
    if (MixedPending && !QuiescentAssist)
    {
        pthread_mutex_lock(&sync);
        require(!retained_calls && !retained_freed && retained_target && dvm_runtime_root_count(retained_target) == 1 &&
                    dvm_refcount_load_u(&retained_target->ref) == 1,
                "unsupported owner stays queued while supported graph physically frees");
        pthread_mutex_unlock(&sync);
    }
    require(!ReplayPressure || (replay_stale && replay_done && replay_refusals == 8 && replay_passes == 8),
            "actual refused snapshot replay pressure and recovery observed");
    pthread_mutex_lock(&sync);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    if (MixedPending)
    {
        native = dvm_runtime_gc_native_try_leave();
        require(native > 0, "parent inactive while unknown callback waits for retained admission");
        deadline = milliseconds() + 5000;
        int done = 0;
        while (milliseconds() < deadline)
        {
            pthread_mutex_lock(&sync);
            done = retained_freed == 1 && retained_calls == 1;
            pthread_mutex_unlock(&sync);
            if (done)
            {
                break;
            }
            fvm_port_thread_sleep_us(1000);
        }
        require(done, "unknown callback remains reachable after shared peers leave");
        dvm_runtime_gc_native_enter(native);
        require(dvm_refcount_load_i(&retained_type->ref) == 1, "unknown heap descriptor owner retired");
        freetype(retained_type);
    }
    fprintf(stderr,
            "MIXEDRETRY retained=%u replayed=%d refusals=%u passes=%u\n",
            retained_freed,
            replay_done,
            replay_refusals,
            replay_passes);
    fprintf(stderr,
            "PASS Fast claim storage nodes=%u modules=%u attempts=%u refused=%u freed=%u native=%u\n",
            NodeCount,
            count,
            allocations,
            refusals,
            freed,
            native_freed);
}
