/* Copyright (c) 2026 bhgv
 * EN: Actual resident peer and native module-reader barriers for the private diagnostic.
 * RU: Барьеры настоящих соседей и native-читателя модуля для частной проверки.
 */


/* EN: Fail an exact semantic predicate using borrowed static message text.
 * RU: Завершить конкретную семантическую проверку; message — статический текст. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL shared module retirement: %s\n", message);
        exit(12);
    }
}


/* EN: Return monotonic milliseconds only for watchdogs, never successful evidence.
 * RU: Вернуть монотонные миллисекунды только для watchdog, не признака успеха. */
static unsigned long long milliseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "monotonic clock available");
    return (unsigned long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}


/* EN: Hold the actual sleep(0) native frame with shared roots. Alternate peer
 * epochs while the other remains admitted; no heap-quiescent pass can collect
 * the fixture. Forward duration unchanged after the parent releases both peers.
 * RU: Удержать настоящий sleep(0) с shared-корнями. Чередовать эпохи, пока другой
 * сосед допущен: проход при общем покое не может собрать объекты проверки.
 * После разрешения родителя передать duration исходным native-кадрам. */
int __wrap_dvm_runtime_guest_sleep(unsigned long duration)
{
    require(!duration && dvm_runtime_gc_dispatch_share(), "actual peer shared admission");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    unsigned long long deadline = milliseconds() + 15000;
    pthread_mutex_lock(&sync);
    unsigned id = admitted++;
    require(id < 2, "exactly two peer residents");
    pthread_cond_broadcast(&changed);
    while (!released && milliseconds() < deadline)
    {
        if (admitted == 2 && turn == id)
        {
            require(dvm_native_owner_leave_shared(owner) && dvm_native_owner_enter_shared(owner),
                    "advance one peer epoch");
            turn = 1 - id;
        }
        pthread_mutex_unlock(&sync);
        fvm_port_thread_sleep_us(1000);
        pthread_mutex_lock(&sync);
    }
    expired |= !released;
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_dispatch_finish();
    return __real_dvm_runtime_guest_sleep(duration);
}


/* [GC:] EN: Receive the exported standalone ticket on this bound native observer.
 * world remains live until join. Two actual reader epochs protect Module.name:
 * the first predates ticket release, the next starts before RC publication while
 * Fast is excluded by sync. Never access metadata outside a registered reader.
 * RU: Получить отдельный ticket на привязанном native-наблюдателе. world живёт
 * до join. Две настоящие эпохи защищают Module.name: первая предшествует снятию
 * ticket, следующая начинается до публикации RC, пока Fast исключён sync.
 * Не обращаться к метаданным вне зарегистрированного чтения.
 */
static void hold_module_reader(void *argument)
{
    DVM_NativeWorld *world = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(unsigned long), NULL);
    require(context && dvm_native_context_bind(context), "module reader binds");
    DVM_WorldParticipant *reader = dvm_world_join_service(world, context);
    require(reader && dvm_world_read_begin(reader), "old module reader begins");
    pthread_mutex_lock(&sync);
    require(!strcmp(target_module->name, ModuleName), "metadata readable through ticket owner");
    volatile uchar *code = __atomic_load_n(&code_identity, __ATOMIC_ACQUIRE);
    uchar first_byte = code ? code[0] : 0;
    require(dvm_owner_transfer_cancel(&ticket), "native observer releases its last ticket alias");
    reader_held = 1;
    pthread_cond_broadcast(&changed);
    while (!reader_release)
    {
        pthread_cond_wait(&changed, &sync);
    }
    require(!strcmp(target_module->name, ModuleName), "metadata survives old ticket reader");
    require(!code || code[0] == first_byte, "native code survives old ticket reader");
    require(dvm_world_read_end(reader), "old module reader ends");
    reader_held = 0;
    require(dvm_world_read_begin(reader), "later module reader begins before RC publication");
    code = __atomic_load_n(&code_identity, __ATOMIC_ACQUIRE);
    later_held = 1;
    pthread_cond_broadcast(&changed);
    while (!later_release)
    {
        pthread_cond_wait(&changed, &sync);
    }
    require(!strcmp(target_module->name, ModuleName), "metadata survives later leaf reader");
    require(!code || code[0] == first_byte, "native code survives later leaf reader");
    require(dvm_world_read_end(reader), "later module reader ends");
    later_held = 0;
    pthread_mutex_unlock(&sync);
    require(dvm_world_detach(reader), "module reader detaches");
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context),
            "module reader context released");
}


/* [GC:] EN: Observe the actual ticket epoch retry, without inspecting payload.
 * context/budget pass unchanged to the real Fast ticket implementation.
 * RU: Наблюдать настоящий повтор эпохи ticket без чтения данных. Передать
 * context/budget неизменными настоящей реализации Fast.
 */
unsigned int __wrap_dvm_managed_owner_drain_shared(DVM_NativeContext *context, size_t budget)
{
    unsigned int work = __real_dvm_managed_owner_drain_shared(context, budget);
    pthread_mutex_lock(&sync);
    if (work && reader_held && dvm_managed_owner_pending() == 1)
    {
        reader_wait_seen = 1;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Observe repeated actual leaf retry responses before releasing its
 * separate reader. No progress is manufactured; normal work passes unchanged.
 * RU: Наблюдать несколько настоящих ответов повтора листа до снятия отдельного
 * читателя. Работа не выдумывается; исходный результат передаётся без изменения.
 */
unsigned int __wrap_dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    unsigned int work = __real_dvm_runtime_rc_shared_leaves(context, budget);
    pthread_mutex_lock(&sync);
    if (work && later_held && handoffs == 1 && !dvm_managed_owner_pending())
    {
        later_passes++;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Reject stopped Fast admission while fixture owners/readers remain.
 * Other collectors keep their existing mixed-runtime policy. Forward world and
 * context unchanged outside this test phase; no managed pointer escapes.
 * RU: Запретить остановленный Fast при живых владельцах/читателях проверки.
 * Другие сборщики сохраняют смешанную политику. Вне этой фазы передать world и
 * context без изменения; управляемый указатель не выходит наружу.
 */
int __wrap_dvm_world_try_quiescent(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    pthread_mutex_lock(&sync);
    require(!(context == fast_context && handoffs && freed != expected_mask && !released),
            "module retirement requested stopped Fast admission");
    pthread_mutex_unlock(&sync);
    return __real_dvm_world_try_quiescent(world, context);
}
