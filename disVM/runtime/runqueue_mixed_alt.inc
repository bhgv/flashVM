/* Copyright (c) 2026 bhgv
 * EN: Track native probe deadlines while residents wait on mixed ALT guards.
 * RU: Учёт сроков нативного опроса при ожидании смешанных веток ALT.
 */


/* EN/RU: See runqueue.h. / См. runqueue.h. */
void dvm_runqueue_arm_mixed_alt(Prog *process, uint64_t deadline)
{
    if (!deadline || process->state != Palt || dvm_runqueue_mixed_alt_deadline(process))
    {
        abort();
    }
    __atomic_fetch_add(&mixed_alt_waiters, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&runtime_process(process)->mixed_alt_deadline, deadline, __ATOMIC_RELEASE);
}


/* EN/RU: See runqueue.h. / См. runqueue.h. */
uint64_t dvm_runqueue_mixed_alt_deadline(Prog *process)
{
    return process ? __atomic_load_n(&runtime_process(process)->mixed_alt_deadline, __ATOMIC_ACQUIRE) : 0;
}


/* EN/RU: See runqueue.h. / См. runqueue.h. */
int dvm_runqueue_mixed_alt_pending(void)
{
    return __atomic_load_n(&mixed_alt_waiters, __ATOMIC_ACQUIRE) != 0;
}
