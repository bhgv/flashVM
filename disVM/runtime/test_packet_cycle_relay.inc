/* Copyright (c) 2026 bhgv
 * EN: Peer relay and scenario selection for packet ownership acceptance.
 * RU: Эстафета соседей и выбор сценария для приёмки владельцев пакета.
 */


/* EN: Fail the exact ownership predicate; message is borrowed constant text.
 * RU: Завершить проверку конкретного владения; message — заимствованный текст. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL packet cycle retirement: %s\n", message);
        exit(12);
    }
}


/* EN: Return monotonic milliseconds for watchdogs, never evidence of success.
 * RU: Вернуть монотонные миллисекунды для watchdog, не признака успеха. */
static unsigned long long milliseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "monotonic clock available");
    return (unsigned long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}


/* EN: Hold real sleep(0) with published shared roots. Two peers alternate
 * epochs while the other stays admitted, preventing any quiescent reclamation.
 * Forward duration unchanged when parent releases the original native frames.
 * RU: Удержать настоящий sleep(0) с shared-корнями. Два соседа чередуют эпохи,
 * пока другой остаётся допущенным, исключая освобождение при общем покое.
 * После разрешения родителя передать duration исходным нативным кадрам. */
int __wrap_dvm_runtime_guest_sleep(unsigned long duration)
{
    require(!duration && dvm_runtime_gc_dispatch_share(), "real peer shared admission");
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


/* EN: Select the positive scenario used by each semantic omission control.
 * RU: Выбрать положительный сценарий соответствующего семантического контроля. */
static unsigned profile(void)
{
    if (CONTROL == 11 || CONTROL == 12)
    {
        return 7;
    }
    if (CONTROL == 9)
    {
        return 3;
    }
    if (CONTROL == 10)
    {
        return 2;
    }
    return CONTROL >= 7 ? 0 : CONTROL;
}


/* [GC:] EN: Gate only until real packet publication creates the intended overlap.
 * context/budget otherwise run the actual Cycles implementation unchanged.
 * RU: Задержать только до настоящей публикации пакета и нужного перекрытия.
 * Иначе выполнить настоящий Cycles с исходными context/budget. */
unsigned int __wrap_dvm_runtime_rc_shared_candidates(DVM_NativeContext *context, size_t budget)
{
    pthread_mutex_lock(&sync);
    int held = envelope && !cycles_open;
    pthread_mutex_unlock(&sync);
    return held ? 1 : __real_dvm_runtime_rc_shared_candidates(context, budget);
}


/* [GC:] EN: Observe actual retry responses before releasing injected pressure.
 * Do not manufacture progress; forward the real context/budget result.
 * RU: Наблюдать настоящие повторы до снятия внедрённого давления. Работу не
 * выдумывать; передать настоящий результат context/budget. */
unsigned int __wrap_dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    packet_retry_observed = 0;
    unsigned int work = __real_dvm_runtime_rc_shared_leaves(context, budget);
    require(!packet_retry_observed || work, "packet slot refusal failed to report shared retry");
    pthread_mutex_lock(&sync);
    if (work && slot_refused && !spills)
    {
        slot_passes++;
    }
    if (work && duplicate_wait && !cycles_open)
    {
        duplicate_passes++;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Reject stopped Fast admission while the graph remains pending.
 * world/context pass unchanged outside the fixture's live ownership interval.
 * RU: Запретить остановленный Fast при ожидающем графе. Вне живого интервала
 * владения передать world/context без изменения. */
int __wrap_dvm_world_try_quiescent(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    pthread_mutex_lock(&sync);
    require(!(context == fast_context && envelope_freed && freed_nodes < node_count && !released),
            "packet cycle requested stopped Fast admission");
    pthread_mutex_unlock(&sync);
    return __real_dvm_world_try_quiescent(world, context);
}
