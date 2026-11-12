/* Copyright (c) 2026 bhgv
 * EN: Native I/O waits and managed publication fault probes.
 * RU: Native I/O-ожидания и отказы управляемой публикации.
 * Private fixture fragment; preserves the original test composition order.
 */


/* Cancel the first of two waiting guests and check that the other still
 * observes reservation completion. Both publish before the mover can reenter. */
static int wait_with_other_guest(Channel *channel, DVM_NativeContext *owner, Prog *process)
{
    int result, cancelled;


    pthread_mutex_lock(&gate);
    if (waiting_count == 2 && getenv("DVM_TEST_DEDICATED_BUFFER"))
    {
        pthread_mutex_unlock(&gate);
        return __real_dvm_runtime_channel_wait_guest(channel);
    }
    check(process && owner != first_owner && waiting_count < 2, "different guest waits on the reservation");
    if (!waiting_count)
    {
        waiting_pid = process->pid;
    }
    cancelled = waiting_pid == process->pid;
    if (++waiting_count == 2)
    {
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
    }
    pthread_mutex_unlock(&gate);

    isolation_cancel_target = cancelled && getenv("DVM_TEST_DEDICATED_BUFFER");
    result = __real_dvm_runtime_channel_wait_guest(channel);
    isolation_cancel_target = 0;
    check(result == (cancelled ? -1 : 1) && dvm_native_context_current() == owner &&
              dvm_native_context_process(owner) == process,
          "only the targeted guest is cancelled");
    puts("PASS isolated channel guest wait");
    return result;
}


/* Check the cancelled native write publishes only the committed prefix. */
FVM_Result
__wrap_dvm_runtime_write_stdio(int descriptor, const void *bytes, size_t capacity, size_t *written, int *released)
{
    FVM_Result result;
    Prog *process;

    if ((stdout_alias_mode == 27 || stdout_alias_mode == 30) && descriptor == 2 && !resume_mover)
    {
        process = dvm_native_context_process(dvm_native_context_current());
        check(process != NULL, "queued alias has a guest owner");
        stdout_wait_pid = process->pid;
        protected_output = stdout_alias_mode == 30;
        result = __real_dvm_runtime_write_stdio(descriptor, bytes, capacity, written, released);
        protected_output = 0;
        check(result != FVM_OK && !*written && !dvm_runtime_cancel_guest_wait(stdout_wait_pid),
              "queued alias cancellation leaves no bytes or wait record");
        pthread_mutex_lock(&gate);
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        return result;
    }
    if (!stdout_cancel_mode || stdout_cancelled || descriptor != 1)
    {
        return __real_dvm_runtime_write_stdio(descriptor, bytes, capacity, written, released);
    }
    process = dvm_native_context_process(dvm_native_context_current());
    check(process != NULL, "stdout wait has a guest owner");
    stdout_wait_pid = process->pid;
    result = __real_dvm_runtime_write_stdio(descriptor, bytes, capacity, written, released);
    check((stdout_cancel_mode == 24 ? stdout_retry_count == 2 && result == FVM_OK && *written == 4096
                                    : result != FVM_OK && !*written) &&
              !dvm_runtime_cancel_guest_wait(stdout_wait_pid),
          "pipe cancellation publishes only committed output and retires the wait");
    stdout_cancelled = 1;
    fputs("PASS cancelled stdout wait\n", stderr);
    return result;
}


/* In the partial case, let the canceller run after the first real splice. */
ssize_t __wrap_splice(
    int source, off64_t *source_offset, int destination, off64_t *destination_offset, size_t size, unsigned int flags)
{
    ssize_t transferred;
    int closes_reader = 0;

    if (stdin_cancel_mode == 22 && source == 0 && !stdin_readiness_raced)
    {
        check((flags & SPLICE_F_NONBLOCK) != 0, "stdin consumes advisory readiness without blocking");
        stdin_readiness_raced = 1;
        errno = EAGAIN;
        return -1;
    }
    if (stdout_cancel_mode == 24 && destination == 1 && stdout_retry_count < 2)
    {
        check((flags & SPLICE_F_NONBLOCK) != 0, "stdout retries without blocking");
        stdout_retry_count++;
        errno = stdout_retry_count == 1 ? EINTR : EAGAIN;
        return -1;
    }
    if (broken_pipe_mode && destination == 1)
    {
        broken_pipe_splices++;
        closes_reader = broken_pipe_splices == (broken_pipe_mode == 28 ? 1 : 2);
        if (closes_reader)
        {
            char acknowledgement;
            fputs("READY close stdout reader\n", stderr);
            check(read(0, &acknowledgement, 1) == 1 && acknowledgement == 'x',
                  "reader closed before the native splice");
        }
    }
    transferred = __real_splice(source, source_offset, destination, destination_offset, size, flags);
    if (closes_reader)
    {
        check(transferred < 0 && errno == EPIPE, "closed pipe reports EPIPE without terminating");
        fputs("PASS closed pipe splice\n", stderr);
        errno = EPIPE;
    }
    if (stdout_alias_mode && destination == 1 && transferred > 0)
    {
        pthread_mutex_lock(&gate);
        if (!first_file_entered)
        {
            first_file_entered = 1;
            pthread_cond_broadcast(&waiting);
            while (!resume_mover)
            {
                pthread_cond_wait(&waiting, &gate);
            }
        }
        pthread_mutex_unlock(&gate);
    }
    if (stdout_cancel_mode == 24 && !stdout_cancelled && destination == 1 && transferred > 0)
    {
        pthread_mutex_lock(&gate);
        alt_wait_entered = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    return transferred;
}


FVM_Result __real_fvm_port_directory_read(FVM_PortDirectory *directory, FVM_FileInfo **entries, size_t *count);


String *__real_c2string(char *value, int length);


/* Hold the first native batch until a second guest has queued on its cursor. */
FVM_Result __wrap_fvm_port_directory_read(FVM_PortDirectory *directory, FVM_FileInfo **entries, size_t *count)
{
    FVM_Result status = __real_fvm_port_directory_read(directory, entries, count);

    if (directory_fault_mode)
    {
        pthread_mutex_lock(&gate);
        directory_native_calls++;
        if (directory_native_calls == 1)
        {
            check(status == FVM_OK && *count, "first native directory batch succeeds");
            first_file_entered = 1;
            pthread_cond_broadcast(&waiting);
            while (!resume_mover)
            {
                pthread_cond_wait(&waiting, &gate);
            }
            directory_fault_owner = dvm_native_context_current();
        }
        pthread_mutex_unlock(&gate);
    }
    return status;
}


/* Fail after the mapped array exists. The original ticket must still precede
 * the competing read, which is already queued. */
String *__wrap_c2string(char *value, int length)
{
    size_t pending;

    if (directory_fault_mode)
    {
        pthread_mutex_lock(&gate);
        if (directory_fault_owner && directory_fault_owner == dvm_native_context_current() &&
            ++directory_fault_strings == 2)
        {
            directory_fault_owner = NULL;
            check(directory_native_calls == 1, "second read cannot overtake publication");
            pthread_mutex_unlock(&gate);
            dvm_file_gate_snapshot(directory_gate, &pending, NULL);
            check(pending == 2, "directory publication retains its original cursor ticket");
            puts("PASS directory publication retains FIFO");
            error("fixture directory publication failure");
        }
        pthread_mutex_unlock(&gate);
    }
    return __real_c2string(value, length);
}
