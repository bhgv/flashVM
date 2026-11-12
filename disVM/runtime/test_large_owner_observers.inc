/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real leaf epochs, retry responses and physical-free observations.
 * RU: Настоящие эпохи листа, ответы повтора и наблюдение физического освобождения.
 */


/* [GC:] EN: Freeze peers before target receives its actual leaf cutoff.
 * value and the existing ownership handoff are forwarded unchanged.
 * RU: Задержать соседей до настоящей эпохи листа target. value и передача
 * существующего владения передаются без изменения. */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    pthread_mutex_lock(&sync);
    publishing_target = target && !target_freed && value == H2D(void *, target);
    pthread_mutex_unlock(&sync);
    if (publishing_target)
    {
        pthread_mutex_lock(&sync);
        if (!target_epoch)
        {
            epochs_held = 1;
        }
        pthread_mutex_unlock(&sync);
    }
    int result = __real_dvm_runtime_rc_defer(value);
    publishing_target = 0;
    return result;
}


/* [GC:] EN: Observe the owned leaf metadata prefix before publication. A
 * matching target identifies its actual cutoff; no queue result is substituted.
 * RU: Прочитать собственный префикс листа до публикации. Совпадение target
 * определяет настоящую эпоху; результат очереди не подменяется. */
int __wrap_dvm_retirement_commit_ready(DVM_RetirementQueue *queue,
                                       DVM_RetirementReservation *token,
                                       void *value,
                                       int *ready)
{
    struct
    {
        Heap *heap;          /* Borrowed producer-owned identity. */
        unsigned long bytes; /* Actual allocation charge. */
        unsigned long epoch; /* Actual leaf cutoff. */
    } prefix;

    if (publishing_target && value && value != H2D(void *, target))
    {
        memcpy(&prefix, value, sizeof(prefix));
        if (prefix.heap == target)
        {
            pthread_mutex_lock(&sync);
            require(prefix.epoch && !target_epoch, "one real parent leaf cutoff");
            target_epoch = prefix.epoch;
            pthread_mutex_unlock(&sync);
        }
    }
    return __real_dvm_retirement_commit_ready(queue, token, value, ready);
}


/* [GC:] EN: Select only the real elapsed target epoch, forwarding world/epoch.
 * Old readers must prevent elapsed; never forge the epoch result.
 * RU: Выбрать лишь настоящую пройденную эпоху target, передав world/epoch.
 * Старые читатели запрещают прохождение; результат эпохи не подменяется. */
int __wrap_dvm_world_epoch_elapsed(DVM_NativeWorld *world, unsigned long epoch)
{
    int result = __real_dvm_world_epoch_elapsed(world, epoch);
    if (fast_call)
    {
        pthread_mutex_lock(&sync);
        int match = target_epoch && epoch == target_epoch;
        epoch_seen |= match;
        selected = match && result && !target_freed;
        require(!match || !epochs_held || !result, "parent epoch waits for actual old readers");
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* [GC:] EN: Observe the private diagnostic boundary after a real leaf failure.
 * heap/retry are production results, never replaced. Supported parents must
 * retain a shared retry even when a competing owner has already disappeared.
 * RU: Проверить диагностическую границу после настоящего отказа листа. heap/retry
 * — ответы runtime без подмены. Поддержанный родитель сохраняет shared-повтор,
 * даже если конкурирующая ссылка уже снята. */
void dvm_test_large_leaf_retry(Heap *heap, int retry)
{
    pthread_mutex_lock(&sync);
    if (heap == target)
    {
        require(retry,
                failure_seen ? "temporary leaf failure must preserve shared retry"
                             : "supported mapped owner must not require retained fallback");
    }
    pthread_mutex_unlock(&sync);
}


/* [GC:] EN: Forward bound context/budget and require actual retry propagation
 * beside retained backlog. Count cutoff waits and injected responses under sync.
 * RU: Передать context/budget и требовать настоящий сигнал повтора рядом с
 * резервной работой. Под sync учесть ожидание эпохи и ответы при отказах. */
unsigned int __wrap_dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    fast_call = 1;
    selected = epoch_seen = failure_seen = 0;
    unsigned int work = __real_dvm_runtime_rc_shared_leaves(context, budget);
    pthread_mutex_lock(&sync);
    require(!failure_seen || work || !dvm_world_has_active_guest(dvm_gc_runtime.world),
            "shared leaf retry survives retained backlog");
    if (epoch_seen && epochs_held)
    {
        held_passes++;
    }
    failure_passes += failure_seen;
    pthread_mutex_unlock(&sync);
    selected = fast_call = 0;
    return work;
}


/* [GC:] EN: Allow the actual unsupported destructor only after guests leave.
 * heap/swept are forwarded to freeheap; Assist releases packet allocation here.
 * RU: Разрешить настоящий неизвестный деструктор лишь после ухода гостей.
 * heap/swept передаются freeheap; Assist снимает отказ памяти пакета здесь. */
static void unknown_destructor(Heap *heap, int swept)
{
    pthread_mutex_lock(&sync);
    DVM_WorldState state;
    require(heap == unknown && released && !unknown_calls && (Assist || (target_freed && freed == Children)) &&
                dvm_world_state(dvm_gc_runtime.world, &state) && state.stopped && !state.active,
            "unknown callback requires actual retained admission");
    unknown_calls++;
    pthread_mutex_unlock(&sync);
    freeheap(heap, swept);
}


/* [GC:] EN: Observe actual selected pool frees exactly once after root/count
 * withdrawal, with peers admitted except in Assist. Generation checks reject
 * reused addresses. pool/value are forwarded unchanged outside fixture sync.
 * RU: Учесть настоящие poolfree ровно один раз после снятия корней/счётчиков
 * при активных соседях, кроме Assist. Поколения исключают повтор адреса.
 * pool/value передаются без изменения вне sync теста. */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    DVM_HeapKey key = dvm_heap_pool_key(pool, value);
    int parent = value == target && key.generation == target_key.generation;
    int child = -1;
    for (unsigned index = 0; index < Children; index++)
    {
        if (value == children[index] && key.generation == child_keys[index].generation)
        {
            child = index;
            break;
        }
    }
    int retained = value == unknown && key.generation == unknown_key.generation;
    if (parent || child >= 0)
    {
        DVM_WorldState state;
        require(!epochs_held && !expired && !dvm_runtime_root_count(value) &&
                    !dvm_refcount_load_u(&((Heap *)value)->ref) && dvm_world_state(dvm_gc_runtime.world, &state) &&
                    !state.stopped && (state.active >= 2 || (Assist && released)),
                "physical frees require shared admission and passed reader epochs");
        if (parent)
        {
            require(!target_freed && !vector && vectors == vector_frees,
                    "parent frees once after all temporary key vectors free");
        }
        else
        {
            require(!destroyed[child], "each distinct child physically frees once");
            destroyed[child] = 1;
        }
    }
    if (retained)
    {
        require(released && unknown_calls == 1 && !unknown_freed && !dvm_runtime_root_count(value),
                "unknown owner frees only after retained callback");
    }
    pthread_mutex_unlock(&sync);
    __real_poolfree(pool, value);
    pthread_mutex_lock(&sync);
    target_freed |= parent;
    freed += child >= 0;
    unknown_freed += retained;
    pthread_mutex_unlock(&sync);
}
