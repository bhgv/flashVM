/* Copyright (c) 2026 bhgv
 * EN: Native handle, sleep and thread startup probes.
 * RU: Проверки native handles, sleep и запуска потоков.
 * Private fixture fragment; preserves the original test composition order.
 */


/* Record the PID before entering the production pipe wait, then check
 * that cancellation leaves no bytes and retires the native wait record. */
FVM_Result __wrap_dvm_runtime_read_stdin(void *bytes, size_t capacity, size_t *received)
{
    FVM_Result result;

    if (!stdin_cancel_mode || stdin_cancelled)
    {
        return __real_dvm_runtime_read_stdin(bytes, capacity, received);
    }
    stdin_wait_pid = ((Prog *)dvm_native_context_process(dvm_native_context_current()))->pid;
    result = __real_dvm_runtime_read_stdin(bytes, capacity, received);
    if (stdin_cancel_mode == 22 && !stdin_partial_received)
    {
        check(stdin_readiness_raced && result == FVM_OK && *received == 1,
              "initial pipe fragment arrives after stale readiness and before cancellation");
        stdin_partial_received = 1;
        return result;
    }
    check(result != FVM_OK && !*received && !dvm_runtime_cancel_guest_wait(stdin_wait_pid),
          "cancelled pipe read consumed no bytes and removed its record");
    stdin_cancelled = 1;
    puts("PASS cancelled stdin poll");
    return result;
}


/* Hold the first real file write until the second leaves the FIFO. */
FVM_Result __wrap_fvm_port_file_write(FVM_PortFile *file, const void *bytes, size_t size)
{
    if ((file_cancel_mode || child_exit_cancel_mode) && size == 1 && *(const char *)bytes == 'a')
    {
        pthread_mutex_lock(&gate);
        first_file_entered = 1;
        pthread_cond_broadcast(&waiting);
        while (!resume_mover)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
    }
    return __real_fvm_port_file_write(file, bytes, size);
}


/* Capture the queued guest identity before native release and verify that
 * it returns while the admitted write still owns the descriptor cursor. */
int __wrap_dvm_file_gate_try_release(DVM_FileGate *file_gate, DVM_FileTicket *ticket)
{
    int queued, result;


    pthread_mutex_lock(&gate);
    queued = file_cancel_mode && first_file_entered && !resume_mover && file_gate;
    if (queued)
    {
        Prog *process = dvm_native_context_process(dvm_native_context_current());
        file_wait_pid = process->pid;
    }
    pthread_mutex_unlock(&gate);
    result = __real_dvm_file_gate_try_release(file_gate, ticket);
    if (queued)
    {
        check(result == -2 && !dvm_runtime_cancel_guest_wait(file_wait_pid),
              "cancelled file wait restores its guest and retires its record");
        pthread_mutex_lock(&gate);
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        puts("PASS cancelled file admission");
    }
    return result;
}


/* The second guest has released NativeWorld and published its reservation. */
int __wrap_dvm_file_gate_enter(DVM_FileGate *file_gate, DVM_FileTicket *ticket)
{
    pthread_mutex_lock(&gate);
    if (directory_fault_mode && !resume_mover)
    {
        if (!directory_gate)
        {
            directory_gate = file_gate;
        }
        else
        {
            check(first_file_entered && directory_gate == file_gate, "second read queues on the same cursor");
            resume_mover = 1;
            pthread_cond_broadcast(&waiting);
        }
    }
    if (stdout_alias_mode && !resume_mover)
    {
        if (!aliased_output_gate)
        {
            aliased_output_gate = file_gate;
        }
        else
        {
            check(first_file_entered && aliased_output_gate == file_gate,
                  "aliased stdout and stderr share the same FIFO");
            if (stdout_alias_mode == 27)
            {
                alt_wait_entered = 1;
            }
            else
            {
                resume_mover = 1;
            }
            pthread_cond_broadcast(&waiting);
        }
    }
    if (file_cancel_mode && first_file_entered && !resume_mover && file_wait_pid)
    {
        alt_wait_entered = 1;
        pthread_cond_broadcast(&waiting);
    }
    pthread_mutex_unlock(&gate);
    {
        int admitted = __real_dvm_file_gate_enter(file_gate, ticket);
        if (admitted && stdout_cancel_mode && stdout_cancel_mode != 24 && stdout_wait_pid && !stdout_cancelled)
        {
            pthread_mutex_lock(&gate);
            alt_wait_entered = 1;
            pthread_cond_broadcast(&waiting);
            pthread_mutex_unlock(&gate);
        }
        if (admitted && stdin_cancel_mode && stdin_wait_pid && !stdin_cancelled &&
            (stdin_cancel_mode != 22 || stdin_partial_received))
        {
            pthread_mutex_lock(&gate);
            alt_wait_entered = 1;
            pthread_cond_broadcast(&waiting);
            pthread_mutex_unlock(&gate);
        }
        return admitted;
    }
}


/* Record the guest PID before the timed condition wait. Verify cleanup
 * after targeted cancellation or a worker failure wakes the sleeper. */
int __wrap_dvm_runtime_guest_sleep(unsigned long milliseconds)
{
    Prog *process;
    int result;

    if (!sleep_mode || milliseconds < 10000)
    {
        return __real_dvm_runtime_guest_sleep(milliseconds);
    }
    process = dvm_native_context_process(dvm_native_context_current());
    check(process != NULL, "sleep has a guest owner");
    sleeping_pid = process->pid;
    if (sleep_mode == 45)
    {
        submit_host_cancel_batch(sleeping_pid);
        verify_sleep_cancel_handoff = 1;
    }
    if (getenv("DVM_TEST_KILLWAIT_BEFORE_SLEEP") || getenv("DVM_TEST_KILL_BEFORE_SLEEP"))
    {
        pthread_mutex_lock(&gate);
        alt_wait_entered = 1;
        pthread_cond_broadcast(&waiting);
        while (!early_killwait_posted)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        puts("PASS exit posted before native sleep registration");
    }
    result = __real_dvm_runtime_guest_sleep(milliseconds);
    check(result == -1 && !dvm_runtime_cancel_guest_wait(sleeping_pid),
          "sleep wakes cancelled and removes its wait record");
    if (verify_sleep_cancel_handoff)
    {
        verify_sleep_cancel_handoff = 0;
        check(dvm_runtime_request_guest_cancel(2000) && !dvm_runtime_request_guest_cancel(2001),
              "published sleep consumes its request and releases the full queue slot");
        puts("PASS native cancellation handoff");
    }
    puts("PASS cancelled native sleep");
    return result;
}


/* Release the companion guest only once the sleeper is about to park. */
int __wrap_fvm_port_sync_wait_ms(FVM_PortSync *sync, unsigned long milliseconds)
{
    if (sleep_mode && milliseconds > 10000)
    {
        pthread_mutex_lock(&gate);
        alt_wait_entered = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    return __real_fvm_port_sync_wait_ms(sync, milliseconds);
}


/* Fail the first replacement thread creation, then allow the retry to
 * create a real worker before releasing the paused mover. */
FVM_PortThread *__wrap_fvm_port_thread_start(FVM_PortThreadFn fn, void *argument)
{
    FVM_PortThread *thread;

    if (!growth_armed)
    {
        return __real_fvm_port_thread_start(fn, argument);
    }
    growth_attempts++;
    if (growth_attempts == 1)
    {
        return NULL;
    }
    check(growth_attempts == 2, "exactly one replacement retry");
    thread = __real_fvm_port_thread_start(fn, argument);
    check(thread != NULL, "replacement thread starts after refusal");
    pthread_mutex_lock(&gate);
    resume_mover = 1;
    pthread_cond_broadcast(&waiting);
    pthread_mutex_unlock(&gate);
    return thread;
}
