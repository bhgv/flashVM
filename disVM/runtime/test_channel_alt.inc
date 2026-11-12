/* Copyright (c) 2026 bhgv
 * EN: Channel wait retries and ALT completion probes.
 * RU: Повторы ожидания каналов и завершения ALT.
 * Private fixture fragment; preserves the original test composition order.
 */


/* Run the same guest wait twice. Refusal must keep the guest attached
 * and remove its native wait record before the next attempt. */
static int wait_with_growth_retry(Channel *channel, DVM_NativeContext *owner, Prog *process, uint64_t generation)
{
    int expected = growth_attempts ? 1 : -2;
    int result;

    check(process && owner != first_owner, "growth retry belongs to the waiting guest");
    growth_armed = 1;
    result =
        channel ? __real_dvm_runtime_channel_wait_guest(channel) : __real_dvm_runtime_channel_wait_change(generation);
    growth_armed = 0;
    check(result == expected && dvm_native_context_current() == owner && dvm_native_context_process(owner) == process &&
              !dvm_runtime_cancel_guest_wait(process->pid),
          "growth attempt preserves continuation and ledger");
    puts("PASS channel wait growth attempt");
    return result;
}


/* Verify that the second guest reaches the production wait on another owner. */
int __wrap_dvm_runtime_channel_wait_guest(Channel *channel)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    Prog *process = dvm_native_context_process(owner);
    int result;
    if (child_exit_cancel_mode || parked_cancel_mode || getenv("DVM_TEST_CLOSE_PARKED"))
    {
        return __real_dvm_runtime_channel_wait_guest(channel);
    }
    if (isolate_waits)
    {
        return wait_with_other_guest(channel, owner, process);
    }
    if (thread_retry)
    {
        return wait_with_growth_retry(channel, owner, process, 0);
    }
    pthread_mutex_lock(&gate);
    if (getenv("DVM_TEST_DEDICATED_BUFFER"))
    {
        while (!first_owner)
        {
            pthread_cond_wait(&waiting, &gate);
        }
    }
    if (resume_mover && getenv("DVM_TEST_DEDICATED_BUFFER"))
    {
        pthread_mutex_unlock(&gate);
        return __real_dvm_runtime_channel_wait_guest(channel);
    }
    check(!alt_bypass, "ALT must not wait when another branch can run");
    check(first_owner && owner != first_owner && process && process != first_process && !resume_mover,
          "second transfer owns a different guest and native context");
    pthread_mutex_unlock(&gate);
    check(!kprotect(try_buffer_finalization, channel) &&
              strstr(getup()->env->errstr, "channel buffer transfer already active") && channel->send && channel->recv,
          "foreign finalizer preserves both queue owners");
    pthread_mutex_lock(&gate);
    if (owner == alt_owner)
    {
        puts("PASS ALT owns the waiting continuation");
    }
    waiting_pid = process->pid;
    if (worker_failure)
    {
        alt_wait_entered = 1;
    }
    else if (!pool_refusal)
    {
        resume_mover = 1;
    }
    pthread_cond_broadcast(&waiting);
    pthread_mutex_unlock(&gate);
    result = __real_dvm_runtime_channel_wait_guest(channel);
    check(result == (pool_refusal ? -2 : (cancel_wait || worker_failure ? -1 : 1)) &&
              dvm_native_context_current() == owner && dvm_native_context_process(owner) == process,
          "same continuation resumes");
    if (worker_failure || pool_refusal)
    {
        pthread_mutex_lock(&gate);
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    if (target_cancel || pool_refusal)
    {
        check(!dvm_runtime_cancel_guest_wait(process->pid), "retired wait has no cancellation record");
    }
    puts("PASS foreign channel guest wait");
    return result;
}


/* Inspect both receive registrations under NativeWorld. This fixture owns
 * both guard channels through its frame and uses no unrelated receivers. */
static void check_retry_alt_queues(void)
{
    uchar *guard = (uchar *)DVM_R.s + offsetof(Alt, ac);
    Channel *channel;
    int index;

    for (index = 0; index < 2; index++)
    {
        memmove(&channel, guard + index * sizeof(Altc) + offsetof(Altc, c), sizeof(channel));
        check(channel && channel != H && channel->recv && !channel->recv->prog,
              "ALT retry has no stale receive registration");
    }
}


/* Verify ALT waits without leaving either guard registered, then let the
 * transfer finish before or during entry to the actual epoch wait. */
int __wrap_dvm_runtime_channel_wait_change(uint64_t generation)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    Prog *process = dvm_native_context_process(owner);
    int result;

    if (thread_retry)
    {
        check(owner == alt_owner, "ALT owns the growth retry");
        check_retry_alt_queues();
        result = wait_with_growth_retry(NULL, owner, process, generation);
        check_retry_alt_queues();
        puts("PASS ALT owns the waiting continuation");
        return result;
    }


    pthread_mutex_lock(&gate);
    check(owner == alt_owner && owner != first_owner && process && process != first_process && !resume_mover,
          "distinct-channel ALT owns the wait");
    waiting_pid = process->pid;
    if (alternate_peer)
    {
        alt_wait_entered = 1;
    }
    else if (!pool_refusal)
    {
        resume_mover = 1;
    }
    pthread_cond_broadcast(&waiting);
    pthread_mutex_unlock(&gate);
    if (target_cancel && getenv("DVM_TEST_DEDICATED_BUFFER"))
    {
        pthread_mutex_lock(&gate);
        while (!dedicated_cancel_posted)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
    }
    result = __real_dvm_runtime_channel_wait_change(generation);
    check(result == (pool_refusal ? -2 : (cancel_wait ? -1 : 1)) && dvm_native_context_current() == owner &&
              dvm_native_context_process(owner) == process,
          "distinct-channel ALT resumes");
    if (pool_refusal)
    {
        pthread_mutex_lock(&gate);
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    if (target_cancel || pool_refusal)
    {
        check(!dvm_runtime_cancel_guest_wait(process->pid), "retired wait has no cancellation record");
    }
    puts("PASS foreign channel guest wait");
    puts("PASS ALT owns the waiting continuation");
    return result;
}


int __real_dvm_runtime_alt_try_transfer(Channel *channel, void *payload, int receive);


int __wrap_dvm_runtime_alt_try_transfer(Channel *channel, void *payload, int receive)
{
    static int forced;
    if (getenv("DVM_TEST_ALT_STALE") && !forced)
    {
        forced = 1;
        WORD contender = receive ? 0 : 11;
        check(__real_dvm_runtime_alt_try_transfer(channel, &contender, receive) &&
                  (contender == 11 || (receive && contender == 31)),
              "change readiness after ALT selected the branch");
        check(!__real_dvm_runtime_alt_try_transfer(channel, payload, receive),
              "stale ALT must not publish a channel wait");
        puts("PASS stale ALT selection refused");
        return 0;
    }
    return __real_dvm_runtime_alt_try_transfer(channel, payload, receive);
}
