/* Copyright (c) 2026 bhgv
 * [GC:] EN: Actual child-packet allocation pressure and competing last claims.
 * RU: Настоящие отказы подготовки пакета и конкуренция за последнюю ссылку.
 */


/* [GC:] EN: Refuse selected fault kind once per real Fast response. Preserve
 * target and every child occurrence; unsupported retained work stays ready.
 * RU: Отказать выбранному kind один раз за настоящий ответ Fast. Сохранить
 * target и все дочерние вхождения; резервная неизвестная работа остаётся готовой. */
static int refuse_selected(int kind)
{
    if (!selected || failure_seen || Fault != kind)
    {
        return 0;
    }
    pthread_mutex_lock(&sync);
    int refuse = Assist ? !unknown_calls : failure_passes < 8;
    if (refuse)
    {
        require(dvm_refcount_load_u(&target->ref) == 1 && dvm_runtime_root_count(target) == 1,
                "temporary failure preserves parent owner and root");
        for (unsigned index = 0; index < Children; index++)
        {
            require(!destroyed[index] && !dvm_runtime_root_count(children[index]) &&
                        dvm_refcount_load_u(&children[index]->ref) == occurrences[index],
                    "temporary failure preserves child roots and alias occurrences");
        }
        refusals++;
        failure_seen = 1;
    }
    pthread_mutex_unlock(&sync);
    return refuse;
}


/* [GC:] EN: Fail exact vector/packet malloc calls after the real parent epoch.
 * bytes is forwarded unchanged otherwise; sparse maps may not allocate a vector
 * by mapped slot count. Track every actual scratch allocation until its free.
 * RU: Отказать точным malloc вектора/пакета после настоящей эпохи родителя.
 * Прочие bytes передаются без изменения; редкая карта не должна выделять вектор
 * по числу слотов. Учесть каждую рабочую память до настоящего free. */
void *__wrap_malloc(size_t bytes)
{
    if (selected && !IsArray && Slots > Owners && bytes == Slots * sizeof(void *))
    {
        require(0, "scratch vector must count live owners");
    }
    int key_vector = selected && !IsArray && Owners > 512 && bytes == Owners * sizeof(void *);
    size_t packet_bytes = sizeof(DVM_RootBatch *) + 2 * sizeof(size_t) + Owners * sizeof(void *);
    if ((key_vector && refuse_selected(VectorMemory)) ||
        (selected && bytes == packet_bytes && refuse_selected(PacketMemory)))
    {
        return NULL;
    }
    void *value = __real_malloc(bytes);
    if (key_vector)
    {
        pthread_mutex_lock(&sync);
        require(value && !vector, "one actual temporary key vector");
        vector = value;
        scratch = value;
        vectors++;
        pthread_mutex_unlock(&sync);
    }
    return value;
}


/* [GC:] EN: Observe scratch free before forwarding the original value.
 * RU: Учесть освобождение рабочей памяти перед передачей исходного value. */
void __wrap_free(void *value)
{
    if (scratch && value == scratch)
    {
        pthread_mutex_lock(&sync);
        vector = NULL;
        scratch = NULL;
        vector_frees++;
        pthread_mutex_unlock(&sync);
    }
    __real_free(value);
}


/* [GC:] EN: Refuse only the actual selected child-root reservation of count.
 * RU: Отказать лишь настоящему резерву count дочерних корней выбранного родителя. */
DVM_RootBatch *__wrap_dvm_runtime_root_batch_create(size_t count)
{
    if (selected && count == Owners && refuse_selected(RootMemory))
    {
        return NULL;
    }
    return __real_dvm_runtime_root_batch_create(count);
}


/* [GC:] EN: Refuse the real queue reservation without modifying queue/bytes/token.
 * RU: Отказать настоящему резерву очереди без изменения queue/bytes/token. */
DVM_RetirementResult
__wrap_dvm_retirement_reserve(DVM_RetirementQueue *queue, unsigned long bytes, DVM_RetirementReservation *token)
{
    if (refuse_selected(PacketSlots) || refuse_selected(PacketBytes))
    {
        return Fault == PacketBytes ? DVM_RETIRE_BYTES : DVM_RETIRE_FULL;
    }
    return __real_dvm_retirement_reserve(queue, bytes, token);
}


/* [GC:] EN: After real child roots publish, acquire an independent parent owner
 * before its actual last claim. batch/keys/count remain production-owned.
 * RU: После настоящей публикации дочерних корней удержать независимую ссылку
 * родителя до его настоящего захвата. batch/keys/count принадлежат runtime. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count)
{
    int result = __real_dvm_runtime_root_batch_publish(batch, keys, count);
    if (Race && selected && result && count == Owners && keys[0] == children[0] && !races)
    {
        require(!extra_owner && dvm_refcount_retain_u(&target->ref) == DVM_REFCOUNT_LIVE &&
                    dvm_runtime_root_add(target),
                "actual competing retain after child roots publish");
        extra_owner = 1;
        race_batch = batch;
    }
    return result;
}


/* [GC:] EN: Verify actual rollback, then withdraw the competing parent owner
 * before the caller's later count observation. No callback result is substituted.
 * RU: Проверить настоящий откат и снять конкурирующего владельца родителя до
 * следующего чтения счётчика вызывающим. Ответы обработчиков не подменяются. */
int __wrap_dvm_runtime_root_batch_clear(DVM_RootBatch *batch)
{
    int result = __real_dvm_runtime_root_batch_clear(batch);
    if (selected && extra_owner && batch == race_batch && result)
    {
        for (unsigned index = 0; index < Children; index++)
        {
            require(!dvm_runtime_root_count(children[index]) &&
                        dvm_refcount_load_u(&children[index]->ref) == occurrences[index],
                    "failed last claim rolls back every child root and reference");
        }
        require(dvm_runtime_root_remove(target) && dvm_refcount_drop_nonfinal_u(&target->ref),
                "competing parent owner ends before later count sample");
        pthread_mutex_lock(&sync);
        races++;
        pthread_mutex_unlock(&sync);
        extra_owner = 0;
        race_batch = NULL;
        failure_seen = 1;
    }
    return result;
}


/* [GC:] EN: Reject omitted root rollback before production destruction can abort.
 * RU: Выявить пропуск отката корней до возможного abort при разрушении batch. */
int __wrap_dvm_runtime_root_batch_destroy(DVM_RootBatch **batch)
{
    require(!selected || !extra_owner || !batch || *batch != race_batch,
            "failed last claim must withdraw child roots before destruction");
    return __real_dvm_runtime_root_batch_destroy(batch);
}
