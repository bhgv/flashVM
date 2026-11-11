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
    Module *lookup = CONTROL == 1 ? lookmod((char *)ModuleName) : NULL;
    Import imports[1] = {{0}};
    Modlink *link = linkmod(module, imports, 1);
    require(link != H && link->MP != H && link->MP != module->origmp, "actual instance MP clone");
    unsigned count = CONTROL == 4 ? 2300 : CONTROL == 3 ? 4 : 2;
    void *values[MaxNodes];
    values[0] = link;
    values[1] = link->MP;
    for (unsigned index = 2; index < count; index++)
    {
        if (CONTROL == 3 && index == 2)
        {
            Module *another = lookmod((char *)ModuleName);
            require(another != NULL, "second actual Module owner");
            Modlink *other = linkmod(another, imports, 1);
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
        if (CONTROL == 3 && index == 2)
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
        if (CONTROL == 3 && index == 3)
        {
            continue;
        }
        require(dvm_refcount_drop_nonfinal_u(&D2H(values[index])->ref), "consume private creator owner");
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
    nodes[count] = D2H(module->origmp);
    graph_count = count;
    node_count = count + 1;
    for (unsigned index = 0; index < node_count; index++)
    {
        keys[index] = dvm_heap_pool_key(heapmem, nodes[index]);
        require(keys[index].generation != 0, "actual graph allocation generation");
    }
    __atomic_store_n(&target_module, module, __ATOMIC_RELEASE);
    live_owner = CONTROL == 2;
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
        int drop_outside = live_owner && live_seen;
        int drop_lookup = lookup && freed == graph_count;
        success = freed == node_count && metadata_freed && !expired;
        pthread_mutex_unlock(&sync);
        if (drop_outside)
        {
            dvm_runtime_gc_native_enter(native);
            require(!freed && link->MP == values[1] && ((void **)values[1])[0] == link,
                    "reachable module cycle retains both original edges");
            pthread_mutex_lock(&sync);
            live_owner = 0;
            pthread_mutex_unlock(&sync);
            require(dvm_owner_transfer_cancel(&outside), "release external native MP owner");
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
    require(success, "module cycle still requires heap quiescence");
    require(confirmed && held_passes >= 8, "real module graph claim and final reader wait observed");
    require(CONTROL != 2 || live_seen, "external MP kept cycle live through actual scan");
    require(CONTROL != 5 || (changed_fields && stale_refused), "actual changed-field snapshot refusal observed");
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_i(&type->ref) == 1, "all graph and Module Type owners physically retired");
    freetype(type);
    pthread_mutex_lock(&sync);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fprintf(stderr,
            "PASS module cycle nodes=%u finalizers=%u metadata=1 final_epoch=1 live=%d stale=%d\n",
            count,
            finalizers,
            live_seen,
            stale_refused);
}
