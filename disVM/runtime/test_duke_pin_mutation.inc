/* Copyright (c) 2026 bhgv
 * [GC:] EN: Detach a real cycle after discovery pins exist and drain the real old-edge seed.
 * RU: Отделить настоящий цикл после захвата pin и снять настоящий seed старого ребра.
 */


/* [GC:] EN: Forward the real Duke slice, then arrange a mutation only after its
 * access ends. start/group/access remain production-owned; native observations
 * inspect no payload after a successful publication or physical free.
 * RU: Выполнить настоящий срез Duke и назначить изменение после окончания
 * access. start/group/access принадлежат runtime; после передачи или free
 * управляемые данные не читаются. */
int __wrap_dvm_rc_duke_capture_slice(void *start, RecordGroup *group, DVM_RangeAccess *access)
{
    int result = __real_dvm_rc_duke_capture_slice(start, group, access);
    pthread_mutex_lock(&sync);
    int selected = node_count && start == H2D(void *, nodes[0]);
    if (selected && result == DVM_DUKE_CAPTURE_RESUME && !mutated)
    {
        require(group->pinned_nodes > 0 && group->nodes[1] == H2D(void *, nodes[1]),
                "actual Duke pin protects the future detached cycle entry");
        __atomic_store_n(&duke_context, dvm_native_context_current(), __ATOMIC_RELEASE);
        mutation_pending = 1;
        mutation_domain = access->domain;
    }
    if (selected && mutated && result == 0)
    {
        require(group->busy, "stale Duke count observation must request retry");
        refused = 1;
    }
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: After the real discovery guard set ends, keep an independent source
 * owner/root, detach its branch with actual write guards and destroy the old field
 * owner through production RC. Wait only for Cycles to consume that queued seed;
 * Duke's still-rooted discovery pin then becomes the last extra owner of the ring.
 * No field guard or registry mutex spans this cross-lane test barrier.
 * RU: После окончания настоящих guards удержать независимый source/корень,
 * отсоединить ветвь под write guards и передать старого владельца настоящему RC.
 * Дождаться снятия seed в Cycles: pin Duke останется последним дополнительным
 * владельцем кольца. Барьер не удерживает guards полей или mutex реестра. */
int __wrap_dvm_range_access_end(DVM_RangeAccess *access)
{
    int result = __real_dvm_range_access_end(access);
    if (!mutation_pending)
    {
        return result;
    }
    mutation_pending = 0;
    require(result, "original Duke discovery guards ended");
    Heap *source = nodes[0];
    require(dvm_runtime_root_add(source) && dvm_refcount_retain_u(&source->ref) == DVM_REFCOUNT_LIVE,
            "independent native source reference and root");
    void **fields = H2D(void **, source);
    DVM_RangeAccess write = {0};
    require(dvm_range_access_begin(mutation_domain, NULL, 0, fields + 1, sizeof(void *), &write) == DVM_CELL_ACQUIRED,
            "actual branch write guards");
    void *old = fields[1];
    require(old == H2D(void *, nodes[1]), "source still owns the real branch edge");
    fields[1] = H;
    require(__real_dvm_range_access_end(&write), "branch write guards ended");
    pthread_mutex_lock(&sync);
    outside_held = 1;
    mutated = 1;
    pthread_mutex_unlock(&sync);
    destroy(old);
    unsigned long long deadline = milliseconds() + 5000;
    for (;;)
    {
        pthread_mutex_lock(&sync);
        int done = seed_drained;
        pthread_mutex_unlock(&sync);
        if (done)
        {
            break;
        }
        require(milliseconds() < deadline, "real mutator retirement seed must reach Cycles before rollback");
        fvm_port_thread_sleep_us(1000);
    }
    return result;
}


/* [GC:] EN: Observe the actual Cycles old-edge seed withdrawal while Duke still
 * owns its separate pin. key is metadata; acquire no managed reference here.
 * RU: Наблюдать снятие настоящего seed старого ребра в Cycles при отдельном
 * удержании Duke. key — метаданные; новая управляемая ссылка не создаётся. */
int __wrap_dvm_runtime_root_remove(const void *key)
{
    int result = __real_dvm_runtime_root_remove(key);
    pthread_mutex_lock(&sync);
    DVM_NativeContext *context = dvm_native_context_current();
    if (result && mutated && !seed_drained && key == nodes[1] && context != duke_context && context != fast_context &&
        !dvm_native_context_process(context))
    {
        require(dvm_runtime_root_count(nodes[1]) == 1 && dvm_refcount_load_u(&nodes[1]->ref) == 2,
                "Duke pin remains after actual old-edge seed consumption");
        seed_drained = 1;
    }
    pthread_mutex_unlock(&sync);
    return result;
}
