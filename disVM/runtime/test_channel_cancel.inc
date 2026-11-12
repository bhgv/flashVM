/* Copyright (c) 2026 bhgv
 * EN: Guest cancellation, child exit and mailbox handoff probes.
 * RU: Отмена гостей, завершение детей и передача через mailbox.
 * Private fixture fragment; preserves the original test composition order.
 */
static void submit_host_cancel_batch(int pid);
static int early_killwait_posted; /* Protected by gate. */


int __real_dvm_runtime_kill_child(Prog *caller, int pid);


int __wrap_dvm_runtime_kill_child(Prog *caller, int pid)
{
    int result = __real_dvm_runtime_kill_child(caller, pid);
    if (getenv("DVM_TEST_KILL_BEFORE_SLEEP"))
    {
        pthread_mutex_lock(&gate);
        early_killwait_posted = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    return result;
}


/* Observe the actual prepared MCALL becoming a scheduler wait. */
int __wrap_dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    int ready = __real_dvm_runtime_prepare_killwait(caller, pid);

    if (!ready && getenv("DVM_TEST_KILLWAIT_BEFORE_SLEEP"))
    {
        pthread_mutex_lock(&gate);
        early_killwait_posted = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    if (child_exit_cancel_mode && !ready)
    {
        pthread_mutex_lock(&gate);
        child_exit_wait_count++;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        if (child_exit_host_cancel && child_exit_wait_count == 1)
        {
            submit_host_cancel_batch(caller->pid);
        }
    }
    return ready;
}


/* EN: Queue over a thousand unique requests before the real target and repeat it.
 * argument borrows a live PID through join. All accepted entries retain FIFO order.
 * RU: Поставить более тысячи запросов перед реальным PID и повторить его.
 * argument заимствует живой PID до join. Принятые запросы сохраняют FIFO. */
static void *submit_host_cancel(void *argument)
{
    int pid = *(int *)argument;
    int index;
    check(!dvm_runtime_cancel_guest_wait(pid), "unbound host cannot touch a scheduler guest");
    if (child_exit_host_cancel && getenv("DVM_TEST_DEDICATED_BUFFER"))
    {
        check(dvm_runtime_request_guest_cancel(pid), "post child wait cancellation");
        return NULL;
    }
    for (index = 0; index < 1024; index++)
    {
        check(dvm_runtime_request_guest_cancel(100000 + index), "growing host request accepted");
    }
    check(dvm_runtime_request_guest_cancel(pid) && dvm_runtime_request_guest_cancel(pid) &&
              dvm_runtime_request_guest_cancel(101023),
          "growing host requests coalesce without refusing admission");
    puts("PASS growing host cancellation batch");
    return NULL;
}


/* Submit on another thread while the controller still owns NativeWorld. */
static void submit_host_cancel_batch(int pid)
{
    pthread_t thread;

    check(pid > 0 && !pthread_create(&thread, NULL, submit_host_cancel, &pid), "host cancellation thread starts");
    check(!pthread_join(thread, NULL), "host cancellation thread joins");
    puts("PASS submitted host cancellation batch");
}


static void submit_host_cancel_once(void)
{
    submit_host_cancel_batch(parked_pid);
    host_cancel_sent = 1;
}


int __real_dvm_runqueue_complete_channel_wait(Prog *process, char *cause);


/* Submit duplicates while delivery still owns the waiter's completion claim. */
static void *submit_during_host_cancel(void *argument)
{
    int pid = *(int *)argument;

    check(dvm_runtime_request_guest_cancel(pid) && dvm_runtime_request_guest_cancel(1062) &&
              !dvm_runtime_request_guest_cancel(2000),
          "current and later requests stay visible while the full batch drains");
    return NULL;
}


int __wrap_dvm_runqueue_complete_channel_wait(Prog *process, char *cause)
{
    if (!getenv("DVM_TEST_DEDICATED_CANCEL") && (host_cancel_mode || host_cancel_on_publish) &&
        !host_cancel_delivery_seen && process->state == Precv && process->pid == parked_pid)
    {
        pthread_t thread;
        int pid = process->pid;

        check(!pthread_create(&thread, NULL, submit_during_host_cancel, &pid),
              "concurrent duplicate request thread starts");
        check(!pthread_join(thread, NULL), "concurrent duplicate request thread joins");
        host_cancel_delivery_seen = 1;
        puts("PASS coalesced cancellation during delivery");
    }
    return __real_dvm_runqueue_complete_channel_wait(process, cause);
}


int __real_dvm_runqueue_claim(Prog *process);


/* A ready guest gets its turn while most of the full host queue still waits. */
int __wrap_dvm_runqueue_claim(Prog *process)
{
    if ((host_cancel_mode || host_cancel_on_publish) && host_cancel_delivery_seen == 1 && process->pid == parked_pid)
    {
        int index;

        for (index = 0; index < 8; index++)
        {
            check(dvm_runtime_request_guest_cancel(3000 + index), "one bounded slice freed eight queue slots");
        }
        check(!dvm_runtime_request_guest_cancel(4000), "ready guest is claimed before a second cancellation slice");
        host_cancel_delivery_seen = 2;
        puts("PASS guest scheduled between cancellation slices");
    }
    return __real_dvm_runqueue_claim(process);
}


int __real_dvm_managed_workers_yield(int idle);


int __wrap_dvm_managed_workers_yield(int idle)
{
    if (host_cancel_no_idle_yield && parked_pid)
    {
        check(!idle && !host_cancel_delivery_seen, "controller must not idle before servicing queued cancellation");
        host_cancel_no_idle_yield = 0;
        puts("PASS cancellation pending at scheduler yield");
    }
    return __real_dvm_managed_workers_yield(idle);
}


/* Force a notification after park captured its generation, but before the
 * controller actually waits. Its old pending observation stays false. */
static int submit_after_park_snapshot(void)
{
    submit_host_cancel_once();
    return 0;
}


/* Drive both sides of the scheduler's no-lost-wake handshake. */
int __wrap_dvm_managed_workers_park(int (*pending)(void))
{
    if (host_cancel_mode && !host_cancel_sent)
    {
        if (host_cancel_mode == 36)
        {
            return __real_dvm_managed_workers_park(submit_after_park_snapshot);
        }
        submit_host_cancel_once();
    }
    return __real_dvm_managed_workers_park(pending);
}


/* Observe a genuine SEND, RECV or ALT registration before the coordinator
 * competes for NativeWorld. The publishing guest completes its turn first. */
void __wrap_dvm_runtime_channel_waiter_published(void)
{
    Prog *process;
    int cancel_pid = 0;

    if (parked_cancel_mode)
    {
        process = dvm_native_context_process(dvm_native_context_current());
        pthread_mutex_lock(&gate);
        if (parked_count < parked_cancel_mode && process &&
            (process->state == Psend || process->state == Precv || process->state == Palt))
        {
            if (!parked_count)
            {
                parked_pid = process->pid;
                if (host_cancel_on_publish)
                {
                    cancel_pid = parked_pid;
                }
            }
            else
            {
                second_parked_pid = process->pid;
            }
            parked_count++;
            pthread_cond_broadcast(&waiting);
        }
        pthread_mutex_unlock(&gate);
    }
    static int close_waiters;
    const char *close_mode = getenv("DVM_TEST_CLOSE_PARKED");
    if (close_mode)
    {
        process = dvm_native_context_process(dvm_native_context_current());
        if (process && process->state == Palt && strcmp(close_mode, "alt-mailbox") == 0)
        {
            Alt *alt = process->R.s;
            if (!dvm_runtime_channel_post_close(alt->ac[0].c))
            {
                error("cannot post test ALT close");
            }
            fprintf(stderr, "TEST posted close to parked ALT\n");
        }
        if (strcmp(close_mode, "multi-mailbox") == 0)
        {
            if (process && process->pid != 1 && (process->state == Precv || process->state == Palt) &&
                __atomic_add_fetch(&close_waiters, 1, __ATOMIC_ACQ_REL) == 2)
            {
                Alt *alt = process->R.s;
                Channel *channel = process->state == Palt ? alt->ac[0].c : process->chan;
                check(dvm_runtime_channel_post_close(channel), "post close to both waiters");
                puts("PASS posted close after both waiters parked");
            }
        }
        else if (process && (process->state == Psend || process->state == Precv) && process->chan && process->chan != H)
        {
            if (strcmp(close_mode, "mailbox") == 0)
            {
                if (!dvm_runtime_channel_post_close(process->chan))
                {
                    error("cannot post test channel close");
                }
            }
            else
            {
                killcomm(process->state == Psend ? &process->chan->send : &process->chan->recv);
            }
        }
    }
    __real_dvm_runtime_channel_waiter_published();
    if (cancel_pid)
    {
        submit_host_cancel_batch(cancel_pid);
    }
}
