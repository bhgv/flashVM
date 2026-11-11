/* Copyright (c) 2026 bhgv
 * EN: Host cancellation delivery and nonblocking standard I/O; included once by main.c.
 * RU: Доставка отмены host и неблокирующий стандартный ввод-вывод; однократно подключается из main.c.
 */


static void post_host_wait_cancel(int pid)
{
    Prog *peer;
    for (peer = dvm_runtime_process_roots(); peer && peer->pid != pid; peer = peer->next)
    {
    }
    if (!peer || (peer->state != Psend && peer->state != Precv && peer->state != Palt &&
                  peer->state != DVM_CHILD_EXIT_WAIT && peer->state != DVM_TABLE_WAIT && !granted_table_retry(peer)))
    {
        return;
    }
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
    /* The owner may still be finishing the turn that published its wait.
     * Leave the message for its next turn. */
    int posted = dvm_messages_send_once(guest_messages, (uintptr_t)pid, &process->channel_cancel);
    if (posted == 1)
    {
        dvm_managed_workers_wake_guest(peer);
    }
}


/* Limit cancellation work before the next guest turn. Keep each PID
 * visible until its delivery finishes so concurrent submissions coalesce. */
static int dispatch_guest_cancellations(void)
{
    size_t remaining;
    int worked;


    fvm_port_sync_lock(channel_queue_mutex);
    remaining = cancel_requests.count;
    worked = remaining != 0;
    while (remaining-- && cancel_requests.count)
    {
        int pid = cancel_requests.pids[0];

        if (native_spawn)
        {
            /* Registration, mailbox publication and queue removal share the
             * ledger mutex. Another resident cannot deliver this PID twice. */
            if (cancel_registered_guest_wait(pid) < 0)
            {
                post_host_wait_cancel(pid);
            }


            dvm_host_cancellations_remove(&cancel_requests, 0);
            continue;
        }
        fvm_port_sync_unlock(channel_queue_mutex);
        dvm_runtime_cancel_guest_wait(pid);
        fvm_port_sync_lock(channel_queue_mutex);
        size_t request = dvm_host_cancellations_find(&cancel_requests, pid);
        if (request < cancel_requests.count)
        {
            dvm_host_cancellations_remove(&cancel_requests, request);
        }
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return worked;
}


/* Admit an audited file operation, with the guest cancellable only until
 * it wins the cursor. On refusal restore the original heap ownership. */
int dvm_file_gate_try_release(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    NativeGuestWait wait;
    int released, admitted;


    dvm_file_gate_reserve(gate, ticket);
    if (!gate)
    {
        return dvm_runtime_gc_native_try_leave();
    }
    native_guest_wait_begin(&wait, gate, ticket, -1);
    released = native_guest_wait_try_leave(&wait);
    if (released < 0)
    {
        dvm_file_gate_cancel(gate, ticket);
        return native_guest_wait_end(&wait, 0) < 0 ? -2 : -1;
    }
    admitted = released ? dvm_file_gate_enter(gate, ticket) : dvm_file_gate_enter_now(gate, ticket);
    native_guest_wait_end(&wait, released);
    if (!admitted)
    {
        dvm_runtime_gc_native_enter(released);
        return released ? -2 : -1;
    }
    return released;
}


/* Wait for the requested pipe readiness or its eventfd cancellation. */
static int wait_stdio_pipe(NativeGuestWait *wait, int wake_fd, int descriptor, short events, int timeout)
{
    struct pollfd descriptors[2] = {{descriptor, events, 0}, {wake_fd, POLLIN, 0}};
    int ready, available;

    do
    {
        ready = poll(descriptors, 2, timeout);
    } while (ready < 0 && errno == EINTR);
    if (ready <= 0 || descriptors[1].revents || (events == POLLOUT && (descriptors[0].revents & (POLLERR | POLLHUP))) ||
        !(descriptors[0].revents & (events == POLLIN ? POLLIN | POLLHUP : POLLOUT)))
    {
        return 0;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    available = !wait->cancelled;
    fvm_port_sync_unlock(channel_queue_mutex);
    return available;
}


/* Decide whether an uncommitted nonblocking pipe transfer should repoll.
 * failure is the saved syscall errno. An interrupt does not establish that
 * a protected caller would block; EAGAIN does for a zero-timeout caller. */
static int retry_stdio_splice(int failure, int timeout)
{
    return failure == EINTR || (failure == EAGAIN && timeout != 0);
}


/* Read a borrowed FIFO through a private pipe without changing its flags.
 * The ledger serializes cancellation against the nonblocking consumption
 * attempt. Advisory readiness alone does not commit the read. */
static FVM_Result read_stdio_pipe(
    NativeGuestWait *wait, int wake_fd, int descriptor, void *bytes, size_t capacity, size_t *received, int timeout)
{
    int staging[2];
    ssize_t count = -1;
    FVM_Result status = FVM_ERROR;

    if (pipe2(staging, O_CLOEXEC | O_NONBLOCK))
    {
        return FVM_OUT_OF_MEMORY;
    }
    while (wait_stdio_pipe(wait, wake_fd, descriptor, POLLIN, timeout))
    {
        int retry;


        fvm_port_sync_lock(channel_queue_mutex);
        if (wait->cancelled)
        {
            fvm_port_sync_unlock(channel_queue_mutex);
            break;
        }
        count = splice(descriptor, NULL, staging[1], NULL, capacity, SPLICE_F_NONBLOCK);
        retry = count < 0 && retry_stdio_splice(errno, timeout);
        if (count >= 0)
        {
            wait->wake_fd = -1;
        }
        fvm_port_sync_unlock(channel_queue_mutex);
        if (!retry)
        {
            break;
        }
    }
    if (count >= 0)
    {
        size_t copied = 0;

        while (copied < (size_t)count)
        {
            ssize_t fragment = read(staging[0], (uchar *)bytes + copied, (size_t)count - copied);
            if (fragment < 0 && errno == EINTR)
            {
                continue;
            }
            if (fragment <= 0)
            {
                break;
            }
            copied += (size_t)fragment;
        }
        *received = copied;
        if (copied == (size_t)count)
        {
            status = FVM_OK;
        }
    }
    close(staging[0]);
    close(staging[1]);
    return status;
}


/* Feed bounded chunks through a private staging pipe. SPLICE_F_NONBLOCK
 * avoids blocking after an advisory POLLOUT even if another writer wins
 * the capacity. Only bytes that reach the borrowed pipe count as written. */
static FVM_Result write_stdio_pipe(NativeGuestWait *wait,
                                   int wake_fd,
                                   int descriptor,
                                   const void *bytes,
                                   size_t capacity,
                                   size_t *written,
                                   int timeout)
{
    const uchar *source = bytes;
    int staging[2];
    size_t staged = 0;
    ssize_t count;

    if (pipe2(staging, O_CLOEXEC | O_NONBLOCK))
    {
        return FVM_OUT_OF_MEMORY;
    }
    while (*written < capacity)
    {
        if (!staged)
        {
            size_t chunk = capacity - *written;
            if (chunk > PIPE_BUF)
            {
                chunk = PIPE_BUF;
            }
            do
            {
                count = write(staging[1], source + *written, chunk);
            } while (count < 0 && errno == EINTR);
            if (count <= 0)
            {
                break;
            }
            staged = (size_t)count;
        }
        if (!wait_stdio_pipe(wait, wake_fd, descriptor, POLLOUT, timeout))
        {
            break;
        }
        count = splice(staging[0], NULL, descriptor, NULL, staged, SPLICE_F_NONBLOCK);
        if (count < 0 && retry_stdio_splice(errno, timeout))
        {
            continue;
        }
        if (count <= 0)
        {
            break;
        }
        staged -= (size_t)count;
        *written += (size_t)count;
    }
    close(staging[0]);
    close(staging[1]);
    return *written || !capacity ? FVM_OK : FVM_ERROR;
}


/* Borrow one gate for each pipe direction. stdout and stderr may refer to
 * the same pipe, so share their gate when the native identities match. */
static DVM_FileGate *stdio_pipe_gate(int descriptor, const struct stat *metadata)
{
    DVM_FileGate *gate;
    struct stat other_metadata;
    int other = 3 - descriptor;
    int shared = descriptor != 0 && !fstat(other, &other_metadata) && S_ISFIFO(other_metadata.st_mode) &&
                 metadata->st_dev == other_metadata.st_dev && metadata->st_ino == other_metadata.st_ino;


    fvm_port_sync_lock(channel_queue_mutex);
    gate = stdio_gates[descriptor];
    if (!gate && shared)
    {
        gate = stdio_gates[other];
    }
    if (!gate)
    {
        gate = dvm_file_gate_create();
    }
    stdio_gates[descriptor] = gate;
    if (shared)
    {
        stdio_gates[other] = gate;
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return gate;
}
