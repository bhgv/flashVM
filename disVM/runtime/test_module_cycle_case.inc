/* Copyright (c) 2026 bhgv
 * EN: Actual module cycle construction and physical-release acceptance.
 * RU: Создание настоящего цикла модуля и приёмка физического освобождения.
 */


/* [GC:] EN: Construct a Module with independent original MP and cloned instance
 * data. Close the instance cycle using real field owners. The large case inserts
 * ordinary records; two-link case consumes two independent registry owners.
 * Keep Type creator until every physical heap and native Module free completes.
 * frame remains production-owned. No fixture reference is silently discarded.
 * RU: Создать Module с отдельным исходным MP и клоном данных экземпляра.
 * Замкнуть цикл настоящими ссылками полей. Большой режим добавляет записи,
 * режим двух link снимает две независимые ссылки реестра. Сохранить создателя Type
 * до всех free кучи/Module. frame принадлежит runtime; ссылки теста не теряются. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    Runtab empty[1] = {{0}};
    Module *module = builtinmod((char *)ModuleName, empty, 0);
    uchar map = 0xc0;
    Type *type = dtype(freeheap, 2 * sizeof(void *), &map, 1);
    require(module && type && dvm_refcount_retain_i(&type->ref) == DVM_REFCOUNT_LIVE, "actual Module and Type owners");
    module->type = mallocz(sizeof(Type *), 1);
    require(module->type != NULL, "actual Module Type vector");
    module->type[0] = type;
    module->ntype = 1;
    module->origmp = H2D(uchar *, heapz(type));
    Module *lookup = (CONTROL == 1 || CONTROL == 12) ? lookmod((char *)ModuleName) : NULL;
    int shared_mp = CONTROL == 10 || CONTROL == 14 || CONTROL == 15 || CONTROL == 16 || CONTROL == 17;
    Import imports[1] = {{0}};
    Modlink *link = linkmod(module, imports, !shared_mp);
    if (shared_mp)
    {
        require(link != H && link->MP == H && dvm_refcount_retain_u(&D2H(module->origmp)->ref) == DVM_REFCOUNT_LIVE,
                "private instance acquires real original-MP owner");
        link->MP = module->origmp;
        Setmark(D2H(link->MP));
    }
    require(link != H && link->MP != H && (shared_mp ? link->MP == module->origmp : link->MP != module->origmp),
            "actual original MP sharing or instance clone");
    unsigned count = (CONTROL == 4 || CONTROL == 17) ? 2300 : (CONTROL == 3 || CONTROL == 16) ? 4 : 2;
    Module *second = NULL;
    void *values[MaxNodes];
    values[0] = link;
    values[1] = link->MP;
    for (unsigned index = 2; index < count; index++)
    {
        if ((CONTROL == 3 || CONTROL == 16) && index == 2)
        {
            Module *another;
            if (CONTROL == 16)
            {
                another = builtinmod((char *)OtherName, empty, 0);
                require(another && dvm_refcount_retain_i(&type->ref) == DVM_REFCOUNT_LIVE,
                        "second native Module Type owner");
                another->type = mallocz(sizeof(Type *), 1);
                require(another->type != NULL, "second Module Type vector");
                another->type[0] = type;
                another->ntype = 1;
                another->origmp = H2D(uchar *, heapz(type));
                second = another;
            }
            else
            {
                another = lookmod((char *)ModuleName);
            }
            require(another != NULL, "second actual Module owner");
            Modlink *other = linkmod(another, imports, CONTROL != 16);
            if (CONTROL == 16)
            {
                require(other != H && other->MP == H &&
                            dvm_refcount_retain_u(&D2H(another->origmp)->ref) == DVM_REFCOUNT_LIVE,
                        "second private instance acquires original-MP owner");
                other->MP = another->origmp;
                Setmark(D2H(other->MP));
            }
            require(other != H && other->MP != H, "second actual module instance");
            values[index] = other;
            values[++index] = other->MP;
        }
        else
        {
            values[index] = H2D(void *, heapz(type));
        }
    }
    for (unsigned index = 1; index < count; index++)
    {
        if ((CONTROL == 3 || CONTROL == 16) && index == 2)
        {
            continue;
        }
        unsigned target = (index + 1) % count;
        require(dvm_refcount_retain_u(&D2H(values[target])->ref) == DVM_REFCOUNT_LIVE, "actual cycle field owner");
        memcpy(values[index], &values[target], sizeof(void *));
        Setmark(D2H(values[target]));
    }
    for (unsigned index = 2; index < count; index++)
    {
        if ((CONTROL == 3 || CONTROL == 16) && index == 3)
        {
            continue;
        }
        require(dvm_refcount_drop_nonfinal_u(&D2H(values[index])->ref), "consume private creator owner");
    }
    if (CONTROL == 11 || CONTROL == 12 || CONTROL == 13)
    {
        require(dvm_refcount_retain_u(&D2H(link)->ref) == DVM_REFCOUNT_LIVE, "original MP owns cycle back edge");
        memcpy(module->origmp, &link, sizeof(link));
        Setmark(D2H(link));
    }
    DVM_OwnerTransfer outside = {0};
    if (CONTROL == 2)
    {
        require(dvm_managed_owner_capture(values[1], &outside), "external native MP owner");
    }
    pthread_mutex_lock(&sync);
    for (unsigned index = 0; index < count; index++)
    {
        nodes[index] = D2H(values[index]);
    }
    graph_count = count;
    node_count = count;
    if (!shared_mp)
    {
        nodes[node_count++] = D2H(module->origmp);
    }
    for (unsigned index = 0; index < node_count; index++)
    {
        keys[index] = dvm_heap_pool_key(heapmem, nodes[index]);
        require(keys[index].generation != 0, "actual graph allocation generation");
    }
    __atomic_store_n(&target_module, module, __ATOMIC_RELEASE);
    __atomic_store_n(&second_module, second, __ATOMIC_RELEASE);
    live_owner = CONTROL == 2 || CONTROL == 12;
    pthread_mutex_unlock(&sync);
    DVM_OwnerTransfer ticket = {0};
    require(dvm_managed_owner_capture(link, &ticket) && dvm_refcount_drop_nonfinal_u(&D2H(link)->ref),
            "seed ticket consumes private Modlink owner");
    require(dvm_owner_transfer_cancel(&ticket), "retire actual module cycle ticket");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent publishes native roots");
    unsigned long long deadline = milliseconds() + 5000;
    int success = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        if (epochs_held && held_passes >= 8)
        {
            require(!freed && !metadata_freed, "old graph reader epochs prevent all target frees");
            epochs_held = 0;
        }
        int drop_outside = live_owner && (live_seen || lookup_refused);
        Module *late = lookup_refused ? racing_lookup : NULL;
        if (late)
        {
            racing_lookup = NULL;
        }
        int drop_lookup = lookup && freed == graph_count;
        success = freed == node_count && metadata_freed == (CONTROL == 16 ? 3 : 1) && !expired;
        int start_leaf = success && (CONTROL == 18 || CONTROL == 19) && !leaf_started;
        if (leaf_gate)
        {
            DVM_GCWorkersState lanes;
            require(dvm_gc_workers_state(dvm_gc_runtime.workers, &lanes), "leaf handoff worker state");
            if (!lanes.Fast.pending && !(lanes.active & (1 << DVM_GC_FAST)) && !dvm_runtime_rc_ready())
            {
                leaf_gate = 0;
            }
        }
        pthread_mutex_unlock(&sync);
        if (start_leaf)
        {
            native = publish_idle_leaf(type, native);
            success = 0;
        }
        if (drop_outside)
        {
            dvm_runtime_gc_native_enter(native);
            require(!freed && link->MP == values[1] && ((void **)values[1])[0] == link,
                    "reachable module cycle retains both original edges");
            pthread_mutex_lock(&sync);
            live_owner = 0;
            pthread_mutex_unlock(&sync);
            if (CONTROL == 2)
            {
                require(dvm_owner_transfer_cancel(&outside), "release external native MP owner");
            }
            else
            {
                unload(late ? late : lookup);
                lookup = NULL;
            }
            native = dvm_runtime_gc_native_try_leave();
            require(native > 0, "parent roots after external release");
        }
        if (drop_lookup)
        {
            dvm_runtime_gc_native_enter(native);
            require(!metadata_freed && dvm_refcount_load_i(&lookup->ref) == 1,
                    "lookup owner keeps Module metadata and original MP");
            unload(lookup);
            lookup = NULL;
            native = dvm_runtime_gc_native_try_leave();
            require(native > 0, "parent roots after Module owner release");
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
        DVM_GCWorkersState lanes;
        require(dvm_gc_workers_state(dvm_gc_runtime.workers, &lanes), "worker diagnostic available");
        fprintf(stderr,
                "module lanes active=%d fast=%d/%zu cycles=%d/%zu duke=%d/%zu ready=%d,%d,%d failed=%d\n",
                lanes.active,
                lanes.Fast.pending,
                lanes.Fast.passes,
                lanes.Cycles.pending,
                lanes.Cycles.passes,
                lanes.Duke.pending,
                lanes.Duke.passes,
                dvm_runtime_rc_ready(),
                dvm_runtime_rc_candidates_ready(),
                dvm_runtime_rc_duke_ready(),
                lanes.failed);
        Module *remaining = __atomic_load_n(&target_module, __ATOMIC_ACQUIRE);
        fprintf(
            stderr,
            "module cycle state freed=%u/%u graph=%u metadata=%d finalizers=%u confirmed=%d epochs=%d passes=%u live=%d lookup=%d native=%d epoch=%d rollback=%d mref=%d\n",
            freed,
            node_count,
            graph_count,
            metadata_freed,
            finalizers,
            confirmed,
            epochs_held,
            held_passes,
            live_owner,
            lookup_refused,
            native_refused,
            epoch_refused,
            rollback_seen,
            remaining ? dvm_refcount_load_i(&remaining->ref) : -1);
        for (unsigned index = 0; index < node_count; index++)
        {
            if (!destroyed[index])
            {
                fprintf(stderr,
                        "module node %u refs=%lu roots=%lu\n",
                        index,
                        dvm_refcount_load_u(&nodes[index]->ref),
                        dvm_runtime_root_count(nodes[index]));
            }
        }
        pthread_mutex_unlock(&sync);
    }
    require(success, "module cycle still requires heap quiescence");
    require(confirmed && held_passes >= 8, "real module graph claim and final reader wait observed");
    require(CONTROL != 2 || live_seen, "external MP kept cycle live through actual scan");
    require(CONTROL != 5 || (changed_fields && stale_refused), "actual changed-field snapshot refusal observed");
    require(CONTROL != 12 || live_seen, "native Module lookup owner kept the original-MP cycle live");
    require(CONTROL != 13 || lookup_refused, "real lookup defeated original-MP graph claim");
    require((CONTROL != 14 && CONTROL != 16) || (native_refused && rollback_seen),
            "native count claim rollback observed");
    require(CONTROL != 15 || (epoch_refused && rollback_seen), "final epoch refusal rollback observed");
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_i(&type->ref) == 1, "all graph and Module Type owners physically retired");
    freetype(type);
    pthread_mutex_lock(&sync);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fprintf(
        stderr,
        "PASS module cycle nodes=%u finalizers=%u metadata=%d final_epoch=1 live=%d stale=%d native_refusal=%d epoch_refusal=%d lookup_refusal=%d rollback=%d\n",
        count,
        finalizers,
        metadata_freed,
        live_seen,
        stale_refused,
        native_refused,
        epoch_refused,
        lookup_refused,
        rollback_seen);
}
