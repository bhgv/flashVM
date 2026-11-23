/* Copyright (c) 2026 bhgv
 * EN: Resume a mixed ALT after its bounded, ordinary local wait.
 * RU: Возобновить смешанный ALT после ограниченного локального ожидания.
 */

static int mixed_alt_probe_active; /* Atomic single holder while local waiters are withdrawn to retry. */
static _Thread_local int mixed_alt_probe_owned; /* Nonzero only on the retried resident's thread. */


/* EN: Sample the host's monotonic clock. Timer management never reads foreign frames.
 * RU: Прочитать монотонные часы хоста. Работа таймера не читает чужие кадры. */
static uint64_t mixed_alt_now(void)
{
    uint64_t now;
    if (!dvm_host_clock_ms(&now))
    {
        abort();
    }
    return now;
}


/* EN: Return a nonzero deadline for an ALT which is about to register locally.
 * RU: Вернуть ненулевой срок для ALT, который собирается ждать локально. */
uint64_t dvm_runtime_mixed_alt_deadline(void)
{
    return mixed_alt_now() + DVM_MIXED_ALT_WAIT_MS;
}


/* EN: Report whether the current resident interrupted its local ALT epoch.
 * RU: Сообщить, прервал ли текущий процесс локальное ожидание ALT. */
int dvm_runtime_mixed_alt_probe_owned(void)
{
    return mixed_alt_probe_owned;
}


/* EN: Release the admission to probe a registered local peer before any RPC.
 * RU: Снять допуск к проверке зарегистрированного локального партнёра до RPC. */
void dvm_runtime_mixed_alt_probe_end(void)
{
    if (mixed_alt_probe_owned)
    {
        mixed_alt_probe_owned = 0;
        __atomic_store_n(&mixed_alt_probe_active, 0, __ATOMIC_RELEASE);
    }
}


/* EN: Confirm the borrowed wait is still pending under the matcher mutex.
 * RU: Под мьютексом сопоставления подтвердить, что этот процесс ещё ждёт. */
static int mixed_alt_waiting(void *state)
{
    Prog *peer = state;
    return dvm_runqueue_observed_state(peer) == Palt && dvm_runqueue_mixed_alt_deadline(peer) &&
        cancellable_channel_wait(peer);
}


/* [GC:] EN: Withdraw this resident's local guards under its exclusive completion
 * claim. Retry by moving the owner-local decoded PC one Dis instruction back.
 * RU: Снять локальные варианты этого процесса под захватом завершения.
 * Для повтора вернуть рабочий PC текущего потока на одну инструкцию Dis. */
static int mixed_alt_retry(void *state)
{
    Prog *peer = state;
    if (!dvm_native_execution()->resident_registers_valid)
    {
        abort();
    }
    dvm_runtime_alt_preflight(peer->R.s);
    altdone(peer->R.s, peer, nil, -1);
    DVM_R.PC--;
    dvm_runtime_channel_wake(peer, nil);
    return 1;
}


/* EN: On the permanent owner, turn one expired waiter into a retry. At most one
 * resident withdraws its local wait until it has had a chance to match peers.
 * RU: На постоянном владельце повторить просроченное ожидание. Лишь один
 * процесс снимает свою регистрацию до попытки сопоставить локальных партнёров. */
static int poll_mixed_alt(Prog *guest)
{
    uint64_t deadline = dvm_runqueue_mixed_alt_deadline(guest);
    if (!deadline || mixed_alt_now() < deadline)
    {
        return 0;
    }
    int free = 0;
    if (!__atomic_compare_exchange_n(&mixed_alt_probe_active, &free, 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
    {
        return 0;
    }
    mixed_alt_probe_owned = 1;
    if (!dvm_runtime_channel_with_queue_claim(guest, mixed_alt_waiting, mixed_alt_retry, guest, NULL))
    {
        dvm_runtime_mixed_alt_probe_end();
        return 0;
    }
    return 1;
}
