/* Copyright (c) 2026 bhgv
 * EN: Actual cyclic records inside a last-owner container produce real retirement packets.
 * RU: Настоящие циклические записи в последнем контейнере создают реальные пакеты освобождения.
 */


/* [GC:] EN: Build a private ring and move the first creator owner into an
 * envelope field. Duplicate mode adds a second real field owner. Only the
 * envelope is captured by a native ticket: target cycle candidates must come
 * from actual field/snapshot packets, not direct test publication. Wait for all
 * physical frees before releasing peers; frame stays production-owned.
 * RU: Построить частное кольцо и передать первую ссылку создателя полю контейнера.
 * Режим дубликата добавляет вторую настоящую ссылку поля. Native ticket захватывает
 * только контейнер: кандидаты цикла должны прийти из настоящих пакетов полей/
 * снимков, не из прямой публикации теста. Дождаться физических free до выхода
 * соседей. frame принадлежит production runtime.
 */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    uchar map = 0x80;
    Type *type = dtype(freeheap, sizeof(void *), &map, 1);
    unsigned records = profile() == 6 ? MaxNodes : 2;
    unsigned fields = profile() == 1 ? 2 : 1;
    uchar container_map = fields == 2 ? 0xc0 : 0x80;
    Type *container_type = dtype(freeheap, fields * sizeof(void *), &container_map, 1);
    require(type && container_type, "actual cyclic and container descriptors");
    void *values[MaxNodes];
    for (unsigned index = 0; index < records; index++)
    {
        values[index] = H2D(void *, heap(type));
    }
    for (unsigned index = 0; index < records; index++)
    {
        void *next = values[(index + 1) % records];
        require(dvm_refcount_retain_u(&D2H(next)->ref) == DVM_REFCOUNT_LIVE, "actual strong cycle edge");
        memcpy(values[index], &next, sizeof(next));
        Setmark(D2H(next));
    }
    void *container = H2D(void *, heap(container_type));
    memcpy(container, &values[0], sizeof(values[0]));
    if (fields == 2)
    {
        require(dvm_refcount_retain_u(&D2H(values[0])->ref) == DVM_REFCOUNT_LIVE, "second actual envelope field owner");
        memcpy((uchar *)container + sizeof(void *), &values[0], sizeof(values[0]));
    }
    for (unsigned index = 1; index < records; index++)
    {
        require(dvm_refcount_drop_nonfinal_u(&D2H(values[index])->ref), "consume non-seed private creator references");
    }
    DVM_OwnerTransfer outside = {0};
    if (profile() == 7)
    {
        require(dvm_managed_owner_capture(values[1], &outside), "independent native cycle owner");
    }
    pthread_mutex_lock(&sync);
    for (unsigned index = 0; index < records; index++)
    {
        nodes[index] = D2H(values[index]);
        keys[index] = __real_dvm_heap_pool_key(heapmem, nodes[index]);
        require(keys[index].generation != 0, "actual target allocation generation");
    }
    node_count = records;
    envelope = D2H(container);
    envelope_key = __real_dvm_heap_pool_key(heapmem, envelope);
    outside_live = profile() == 7;
    cycles_open = profile() == 4 || profile() == 5;
    pthread_mutex_unlock(&sync);
    DVM_OwnerTransfer ticket = {0};
    require(dvm_managed_owner_capture(container, &ticket) && dvm_refcount_drop_nonfinal_u(&D2H(container)->ref),
            "ticket owns last envelope reference");
    require(dvm_owner_transfer_cancel(&ticket), "release actual native envelope ticket");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent native roots published");
    unsigned long long deadline = milliseconds() + 5000;
    int success = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        if (profile() == 1 ? spills && duplicate_wait && duplicate_passes >= 8 : spills != 0)
        {
            cycles_open = 1;
        }
        int release_outside = outside_live && live_seen;
        success = envelope_freed && freed_nodes == node_count && !expired;
        pthread_mutex_unlock(&sync);
        if (release_outside)
        {
            dvm_runtime_gc_native_enter(native);
            require(dvm_runtime_root_count(nodes[1]) == 1 && !freed_nodes,
                    "reachable cycle survives actual Cycles processing");
            void *first_edge;
            void *second_edge;
            memcpy(&first_edge, values[0], sizeof(first_edge));
            memcpy(&second_edge, values[1], sizeof(second_edge));
            require(first_edge == values[1] && second_edge == values[0], "reachable cycle fields remain intact");
            pthread_mutex_lock(&sync);
            outside_live = 0;
            pthread_mutex_unlock(&sync);
            require(dvm_owner_transfer_cancel(&outside), "release last external native graph owner");
            native = dvm_runtime_gc_native_try_leave();
            require(native > 0, "parent publishes roots after live-owner release");
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
        fprintf(stderr,
                "packet state spills=%u live=%d outside=%d freed=%u envelope=%d\n",
                spills,
                live_seen,
                outside_live,
                freed_nodes,
                envelope_freed);
        for (unsigned index = 0; index < records; index++)
        {
            fprintf(stderr,
                    "node %u refs=%u roots=%zu indexed=%d\n",
                    index,
                    dvm_refcount_load_u(&nodes[index]->ref),
                    dvm_runtime_root_count(nodes[index]),
                    __real_dvm_retirement_contains_key(candidate_queue, keys[index]));
        }
        pthread_mutex_unlock(&sync);
    }
    require(success, "packet cycle still requires heap quiescence");
    require((profile() != 2 && profile() != 5) || root_refused, "replacement-root OOM was not exercised");
    require(profile() != 3 || (slot_refused && slot_passes >= 8), "mandatory slot pressure was not exercised");
    require(profile() != 7 || live_seen, "live outside-owner scan was not exercised");
    require((profile() != 4 && profile() != 5) || snapshot_refused, "actual snapshot refusal was not exercised");
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_i(&type->ref) == 1 && dvm_refcount_load_i(&container_type->ref) == 1,
            "all target physical owners released their descriptors");
    freetype(type);
    freetype(container_type);
    pthread_mutex_lock(&sync);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fprintf(
        stderr,
        "PASS packet cycle records=%u envelope=1 spills=%u root_refusal=%d slot_refusal=%d snapshot_refusal=%d duplicate=%d live=%d\n",
        records,
        spills,
        root_refused,
        slot_refused,
        snapshot_refused,
        duplicate_wait,
        live_seen);
}
