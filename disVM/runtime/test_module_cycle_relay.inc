/* Copyright (c) 2026 bhgv
 * EN: Peer admission relay and old reader epochs for actual module cycles.
 * RU: Эстафета допуска соседей и старые эпохи читателей настоящих циклов модулей.
 */


/* EN: Fail the exact ownership predicate; message is borrowed constant text.
 * RU: Завершить проверку конкретного владения; message — заимствованный текст. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL module cycle retirement: %s\n", message);
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
        if (admitted == 2 && turn == id && !epochs_held)
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
