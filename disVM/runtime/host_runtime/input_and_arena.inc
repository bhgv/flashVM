/* Copyright (c) 2026 bhgv
 * EN: Input waiting, channel queue guards and heap arena startup; included once by main.c.
 * RU: Ожидание ввода, защита очередей каналов и запуск арены кучи; однократно подключается из main.c.
 */


/* A pipe has a cancellable readiness phase. The borrowed descriptor is never
 * reconfigured. Return to the original heap owner on every path. */
static FVM_Result runtime_stdio_transfer(
    int descriptor, void *input, const void *output, size_t capacity, size_t *transferred, int *released_out)
{
    struct stat metadata;
    NativeGuestWait wait;
    DVM_FileTicket ticket;
    DVM_FileGate *gate;
    FVM_Result status = FVM_ERROR;
    int wake_fd, released;

    *released_out = 0;
    *transferred = 0;
    if (!capacity || fstat(descriptor, &metadata) || !S_ISFIFO(metadata.st_mode))
    {
        released = dvm_runtime_gc_native_try_leave();
        if (released < 0)
        {
            return FVM_OUT_OF_MEMORY;
        }
        status = input ? fvm_port_stdio_read(descriptor, input, capacity, transferred)
                       : fvm_port_stdio_write(descriptor, output, capacity, transferred);
        dvm_runtime_gc_native_enter(released);
        *released_out = released;
        return status;
    }
    gate = stdio_pipe_gate(descriptor, &metadata);
    if (!gate)
    {
        return FVM_OUT_OF_MEMORY;
    }
    wake_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (wake_fd < 0)
    {
        return FVM_OUT_OF_MEMORY;
    }
    dvm_file_gate_reserve(gate, &ticket);
    native_guest_wait_begin(&wait, gate, &ticket, wake_fd);
    released = native_guest_wait_try_leave(&wait);
    if (released < 0)
    {
        dvm_file_gate_cancel(gate, &ticket);
        status = released == -2 ? FVM_ERROR : FVM_OUT_OF_MEMORY;
    }
    else
    {
        int timeout = !released && gc_worker ? 0 : -1;
        int admitted = released ? dvm_file_gate_enter(gate, &ticket) : dvm_file_gate_enter_now(gate, &ticket);

        if (admitted)
        {
            if (input)
            {
                status = read_stdio_pipe(&wait, wake_fd, descriptor, input, capacity, transferred, timeout);
            }
            else
            {
                status = write_stdio_pipe(&wait, wake_fd, descriptor, output, capacity, transferred, timeout);
            }
            if (!dvm_file_gate_leave(gate, &ticket))
            {
                abort();
            }
        }
        dvm_runtime_gc_native_enter(released);
        *released_out = released;
    }
    native_guest_wait_end(&wait, released);
    close(wake_fd);
    return status;
}


/* Read staged stdin bytes and return after restoring the guest continuation. */
FVM_Result dvm_runtime_read_stdin(void *bytes, size_t capacity, size_t *received)
{
    int released;

    return runtime_stdio_transfer(0, bytes, NULL, capacity, received, &released);
}


/* Write staged stdout/stderr bytes. Return any committed prefix, even if a
 * later pipe fragment is cancelled. released reports a completed heap lease. */
FVM_Result dvm_runtime_write_stdio(int descriptor, const void *bytes, size_t capacity, size_t *written, int *released)
{
    return runtime_stdio_transfer(descriptor, NULL, bytes, capacity, written, released);
}


/* Serialize short queue operations and unlock before propagating a VM error. */
void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument)
{
    int completed;
    fvm_port_sync_lock(channel_queue_mutex);
    completed = kprotect(operation, argument);
    fvm_port_sync_unlock(channel_queue_mutex);
    if (!completed)
    {
        krethrow();
    }
}


extern int dvm_runtime_channel_buffer_busy(Channel *channel);


/* EN: Read a borrowed live channel's reservation state under its queue mutex.
 * GC callers must not traverse stack reservation records without this lock.
 * The result is an observation; a later claim must revalidate. No heap access.
 * RU: Прочитать состояние резервации живого заимствованного канала под мьютексом.
 * GC не обходит стековые записи без блокировки. Это наблюдение, последующий
 * захват перепроверяет состояние. Управляемая память здесь не разыменовывается. */
int dvm_runtime_channel_buffer_busy_snapshot(Channel *channel)
{
    fvm_port_sync_lock(channel_queue_mutex);
    int busy = dvm_runtime_channel_buffer_busy(channel);
    fvm_port_sync_unlock(channel_queue_mutex);
    return busy;
}


/* Wait on the native reservation ledger only. The caller keeps the channel
 * identity live and must have no bound VM context. Idle is an observation,
 * so the caller must still revalidate when it later acquires a reservation. */
static int wait_channel_buffer_idle(Channel *channel, NativeGuestWait *wait)
{
    int completed;
    fvm_port_sync_lock(channel_queue_mutex);
    while (!native_waits_stopping && (!wait || !wait->cancelled) && dvm_runtime_channel_buffer_busy(channel))
    {
        channel_buffer_waiters++;
        fvm_port_sync_wait(channel_queue_mutex);
        channel_buffer_waiters--;
    }
    completed = !native_waits_stopping;
    fvm_port_sync_unlock(channel_queue_mutex);
    return completed;
}


/* Unbound native callers use only the reservation ledger. */
int dvm_runtime_channel_wait_idle(Channel *channel)
{
    if (!channel || channel == H || dvm_native_context_current())
    {
        return 0;
    }
    return wait_channel_buffer_idle(channel, NULL);
}


/* The caller holds a rooted channel and has ruled out a mover dependency.
 * Native leave withdraws the current Prog until the same owner resumes it. */
int dvm_runtime_channel_wait_guest(Channel *channel)
{
    NativeGuestWait wait;
    int released;


    native_guest_wait_begin(&wait, NULL, NULL, -1);
    released = native_guest_wait_try_leave(&wait);
    if (released > 0)
    {
        wait_channel_buffer_idle(channel, &wait);
        dvm_runtime_gc_native_enter(released);
    }
    return native_guest_wait_end(&wait, released < 0 ? -2 : released);
}


/* Snapshot before inspecting guards so any later publication forces a retry. */
uint64_t dvm_runtime_channel_snapshot(void)
{
    fvm_port_sync_lock(channel_queue_mutex);
    uint64_t generation = channel_generation;
    fvm_port_sync_unlock(channel_queue_mutex);
    return generation;
}


/* The caller holds the channel queue mutex. */
int dvm_runtime_channel_changed_locked(uint64_t generation)
{
    return generation != channel_generation;
}


/* Wait for a change since the guard scan. ALT registrations are withdrawn. */
int dvm_runtime_channel_wait_change(uint64_t generation)
{
    NativeGuestWait wait;
    native_guest_wait_begin(&wait, NULL, NULL, -1);
    int released = native_guest_wait_try_leave(&wait);

    if (released > 0)
    {
        fvm_port_sync_lock(channel_queue_mutex);
        channel_buffer_waiters++;
        while (!native_waits_stopping && !wait.cancelled && generation == channel_generation)
        {
            fvm_port_sync_wait(channel_queue_mutex);
        }
        channel_buffer_waiters--;
        fvm_port_sync_unlock(channel_queue_mutex);
        dvm_runtime_gc_native_enter(released);
    }
    return native_guest_wait_end(&wait, released < 0 ? -2 : released);
}


/* Wait for a monotonic duration on the native cancellation ledger. The
 * scalar duration is private and all VM state stays in the saved frame. */
int dvm_runtime_guest_sleep(unsigned long milliseconds)
{
    NativeGuestWait wait;
    uint64_t started, now, elapsed;
    int released, completed = milliseconds == 0;


    native_guest_wait_begin(&wait, NULL, NULL, -1);
    released = native_guest_wait_try_leave(&wait);
    if (released > 0)
    {
        fvm_port_sync_lock(channel_queue_mutex);
        if (milliseconds && dvm_host_clock_ms(&started))
        {
            now = started;
            while (!native_waits_stopping && !wait.cancelled)
            {
                elapsed = now - started;
                if (elapsed > milliseconds)
                {
                    completed = 1;
                    break;
                }
                if (!fvm_port_sync_wait_ms(channel_queue_mutex, milliseconds - elapsed + 1) || !dvm_host_clock_ms(&now))
                {
                    break;
                }
            }
        }
        fvm_port_sync_unlock(channel_queue_mutex);
        dvm_runtime_gc_native_enter(released);
    }
    return native_guest_wait_end(&wait, released ? (completed ? 1 : -1) : 0);
}


/* Translate a guest wait result after its native continuation is restored.
 * A cancelled wait has a distinct guest-visible exception. */
void dvm_runtime_channel_require_wait(int outcome)
{
    if (outcome == -2)
    {
        error("channel wait needs an executor");
    }
    if (outcome < 0)
    {
        error("channel wait cancelled");
    }
    if (!outcome)
    {
        error("channel buffer transfer already active");
    }
}


/* Advance readiness after queue publication or reservation removal, under the mutex. */
void dvm_runtime_channel_buffer_notify(void)
{
    channel_generation++;
    if (channel_buffer_waiters)
    {
        fvm_port_sync_signal_all(channel_queue_mutex);
    }
}


/* Notify selectors once a guest waiter is committed. Temporary ALT queue
 * registrations are invisible to other selectors under NativeWorld. */
void dvm_runtime_channel_waiter_published(void)
{
    if (!dvm_runtime_guarded_copies)
    {
        return;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    dvm_runtime_channel_buffer_notify();
    fvm_port_sync_unlock(channel_queue_mutex);
}


/* Cancel native guest waits before joining workers. A suspended guest
 * still reacquires NativeWorld and restores its continuation before unwind. */
void dvm_runtime_cancel_native_waits(void)
{
    if (channel_queue_mutex)
    {
        fvm_port_sync_lock(channel_queue_mutex);
        native_waits_stopping = 1;
        cancel_requests_open = 0;
        dvm_host_cancellations_clear(&cancel_requests);
        for (NativeGuestWait *wait = native_guest_waits; wait; wait = wait->next)
        {
            cancel_guest_wait(wait);
        }
        fvm_port_sync_signal_all(channel_queue_mutex);
        fvm_port_sync_unlock(channel_queue_mutex);
    }
}


/* Create arena/mutex privately; VM allocation errors occur outside pool lock. */
static void arena_init(void)
{
    native_waits_stopping = 0;
    channel_queue_mutex = fvm_port_sync_create();
    if (!channel_queue_mutex || !dvm_heap_pool_init(&memory, ARENA_BYTES))
    {
        error(exNomem);
    }
    dvm_runqueue_ready_sync(channel_queue_mutex);
}


/* Allocate ordinary host metadata; the VM heap uses its separate bounded arena. */
void *mallocz(ulong size, int clear)
{
    void *value = malloc(size);
    if (value && clear)
    {
        memset(value, 0, size);
    }
    return value;
}
