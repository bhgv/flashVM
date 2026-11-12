/* Copyright (c) 2026 bhgv
 * EN: Deterministic Cycles-to-idle-Fast final-owner notification acceptance.
 * RU: Детерминированная проверка уведомления простаивающего Fast из Cycles.
 */


/* [GC:] EN: Observe the actual leaf replacement root added by Cycles for key.
 * Forward ownership-neutral registry work; Fast and guest producers are ignored.
 * RU: Наблюдать настоящий замещающий корень листа key от Cycles. Передать
 * операцию реестра без изменения владения; Fast/гостевые производители игнорируются. */
int __wrap_dvm_runtime_root_add(void *key)
{
    int result = __real_dvm_runtime_root_add(key);
    pthread_mutex_lock(&sync);
    if (result && key == leaf_target && dvm_native_context_current() != fast_context &&
        !dvm_native_context_process(dvm_native_context_current()))
    {
        leaf_published = 1;
        leaf_notified = 0;
    }
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: Observe the real scheduler request for workers/kind; forward unchanged.
 * No work or readiness is manufactured by this hook.
 * RU: Наблюдать настоящий запрос планировщика workers/kind без изменения.
 * Наблюдатель не создаёт готовность или выдуманную работу. */
int __wrap_dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    int result = __real_dvm_gc_workers_request(workers, kind);
    if (result && kind == DVM_GC_FAST && leaf_published)
    {
        leaf_notified = 1;
    }
    return result;
}


/* [GC:] EN: Keep the independent leaf candidate gated until parent observes
 * idle Fast and empty Fast queues. Then run the real Cycles pass and require its
 * leaf transition to notify Fast before returning. context/budget stay real.
 * RU: Удержать отдельного кандидата до наблюдения простоя Fast и пустых его
 * очередей. Затем выполнить настоящий проход Cycles и потребовать уведомления
 * Fast при передаче листа до возврата. context/budget остаются настоящими. */
unsigned int __wrap_dvm_runtime_rc_shared_candidates(DVM_NativeContext *context, size_t budget)
{
    pthread_mutex_lock(&sync);
    int held = leaf_gate;
    pthread_mutex_unlock(&sync);
    if (held)
    {
        return 1;
    }
    unsigned int work = __real_dvm_runtime_rc_shared_candidates(context, budget);
    require(!leaf_published || leaf_notified, "Cycles final-owner publication must notify idle Fast");
    leaf_published = 0;
    return work;
}


/* [GC:] EN: While parent owns native admission state, reenter and publish one
 * actual mapped record to Cycles with two real references, then consume only the
 * private extra owner. Gate Cycles until Fast is idle; the queue's sole remaining
 * reference must become an actual leaf. Return parent's new native interval.
 * type remains creator-owned; its real heap is added to physical-free observers.
 * RU: Вернуться в управляемый допуск родителя и передать Cycles настоящую запись
 * с двумя ссылками, затем снять только частного дополнительного владельца.
 * Удержать Cycles до простоя Fast; единственная ссылка очереди станет листом.
 * Вернуть новый native-интервал родителя. type удержан создателем; free наблюдается. */
static int publish_idle_leaf(Type *type, int native)
{
    dvm_runtime_gc_native_enter(native);
    void *value = H2D(void *, heapz(type));
    require(dvm_refcount_retain_u(&D2H(value)->ref) == DVM_REFCOUNT_LIVE, "actual private leaf test owner");
    pthread_mutex_lock(&sync);
    leaf_target = D2H(value);
    leaf_gate = 1;
    leaf_started = 1;
    require(node_count < MaxNodes, "physical leaf target capacity");
    nodes[node_count] = leaf_target;
    keys[node_count++] = dvm_heap_pool_key(heapmem, leaf_target);
    pthread_mutex_unlock(&sync);
    require(dvm_runtime_rc_defer(value), "publish actual nonfinal leaf candidate");
    require(dvm_refcount_drop_nonfinal_u(&D2H(value)->ref), "consume only private extra leaf owner");
    int result = dvm_runtime_gc_native_try_leave();
    require(result > 0, "native parent waits for cross-lane leaf free");
    return result;
}
