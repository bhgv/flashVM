/* Copyright (c) 2026 bhgv
 * EN: Inspection, native file owners and cancellation readiness; included once by main.c.
 * RU: Инспекция, владельцы нативных файлов и готовность отмены; однократно подключается из main.c.
 */


/* Report copied signed native words or binary64 after all processes have stopped, then release the retain.
 * No Dis stack/frame address is exposed. Returns zero on output failure; never changes module data. */
static int inspect_report(void)
{
    size_t index;
    int ok = 1;
    for (index = 0; index < inspect_count; index++)
    {
        int written; /* Host output status; either view copies bytes without aliasing typed storage. */
        size_t offset = inspect_offset + index * 8U;
        if (inspect_reals)
        {
            double value; /* NC64 REAL uses the host binary64 representation admitted by this runtime. */
            memcpy(&value, inspect_data + offset, sizeof(value));
            written = printf("REAL %zu %.17g\n", offset, value);
        }
        else
        {
            int64_t value;
            memcpy(&value, inspect_data + offset, sizeof(value));
            written = printf("DATA %zu %" PRId64 "\n", offset, value);
        }
        if (written < 0)
        {
            ok = 0;
            break;
        }
    }
    inspect_release();
    return ok;
}


/* Report allocator corruption and terminate instead of continuing with invalid metadata. */
static void allocator_fault(void *address, char *reason, ulong pc)
{
    USED(address);
    USED(pc);
    fprintf(stderr, "disvm allocator: %s\n", reason);
    exit(1);
}

void (*poolfault)(void *, char *, ulong) = allocator_fault; /* Production D2B guard hook. */


static DVM_HostCancellations cancel_requests; /* Growing PID FIFO under the channel mutex. */
static int cancel_requests_open;              /* NativeWorld scheduler is live, under the channel mutex. */

static FVM_PortSync *channel_queue_mutex;     /* Lives for the hosted runtime. */
static unsigned int channel_buffer_waiters;   /* Protected by the queue mutex. */
static int native_waits_stopping;             /* Shutdown predicate under the queue mutex. */
static DVM_FileGate *stdio_gates[3];          /* Output aliases share an admission queue. */
static uint64_t channel_generation;           /* Queue/readiness epoch protected by the same mutex. */


typedef struct NativeGuestWait
{
    int pid;                      /* Copied from the current guest before releasing NativeWorld. */
    int cancelled;                /* Protected by the queue mutex. */
    DVM_FileGate *file_gate;      /* Borrowed through descriptor pin, or NULL. */
    int wake_fd;                  /* Owned by a pipe reader until it unlinks this record. */
    DVM_FileTicket *file_ticket;  /* Live caller storage through queue admission. */
    struct NativeGuestWait *next; /* Linked only while this stack record is live. */
} NativeGuestWait;

static NativeGuestWait *native_guest_waits; /* Protected by the queue mutex. */


static int cancel_guest_wait(NativeGuestWait *wait);


/* Register the current guest before suspension and consume any pending cancellation. */
static void native_guest_wait_begin(NativeGuestWait *wait, DVM_FileGate *gate, DVM_FileTicket *ticket, int wake_fd)
{
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    size_t request;

    wait->pid = process ? process->pid : 0;
    wait->cancelled = 0;
    wait->file_gate = gate;
    wait->file_ticket = ticket;
    wait->wake_fd = wake_fd;
    fvm_port_sync_lock(channel_queue_mutex);
    wait->next = native_guest_waits;
    native_guest_waits = wait;
    request = dvm_host_cancellations_find(&cancel_requests, wait->pid);
    if (request < cancel_requests.count)
    { /* Transfer the queued request to this exact continuation. */
        dvm_host_cancellations_remove(&cancel_requests, request);
        cancel_guest_wait(wait);
    }
    /* A dedicated exit can arrive after the instruction starts but before
     * its native wait is linked. Observe that request before blocking. */
    if (native_spawn && process && (__atomic_load_n(&process->flags, __ATOMIC_ACQUIRE) & Pkilled))
    {
        cancel_guest_wait(wait);
    }
    if (native_waits_stopping && gate && !wait->cancelled)
    {
        wait->cancelled = dvm_file_gate_cancel(gate, ticket);
    }
    fvm_port_sync_unlock(channel_queue_mutex);
}


/* Skip native admission when cancellation has already won publication.
 * wait borrows a registered stack record. The ledger is released before
 * requesting a heap transition. Return -2 for early cancellation, otherwise
 * the audited release result; a later cancellation still wakes the waiter. */
static int native_guest_wait_try_leave(NativeGuestWait *wait)
{
    int cancelled;


    fvm_port_sync_lock(channel_queue_mutex);
    cancelled = native_waits_stopping || wait->cancelled;
    fvm_port_sync_unlock(channel_queue_mutex);
    if (cancelled)
    {
        return -2;
    }
    return dvm_runtime_gc_native_try_leave();
}


/* Remove the record after restoring NativeWorld. Cancellation can still
 * win while the guest is reacquiring its continuation. */
static int native_guest_wait_end(NativeGuestWait *wait, int released)
{
    NativeGuestWait **link = &native_guest_waits;
    int completed;


    fvm_port_sync_lock(channel_queue_mutex);
    while (*link != wait)
    {
        link = &(*link)->next;
    }
    *link = wait->next;
    completed = native_waits_stopping || wait->cancelled ? -1 : released;
    fvm_port_sync_unlock(channel_queue_mutex);
    return completed;
}


/* With the ledger locked, cancel only operations still queued for I/O.
 * File gates never call into the ledger while holding their own mutex. */
static int cancel_guest_wait(NativeGuestWait *wait)
{
    if (wait->cancelled)
    {
        return 1;
    }
    if (wait->file_gate && !dvm_file_gate_cancel(wait->file_gate, wait->file_ticket) && wait->wake_fd < 0)
    {
        return 0;
    }
    wait->cancelled = 1;
    if (wait->wake_fd >= 0)
    {
        int status;
        do
        {
            status = eventfd_write(wait->wake_fd, 1);
        } while (status < 0 && errno == EINTR);
        if (status < 0 && errno != EAGAIN)
        {
            abort();
        }
    }
    return 1;
}


static int cancel_scheduler_guest_wait(int pid);


/* Find and cancel a registered continuation while holding the ledger lock.
 * A negative result means the PID has no native wait. */
static int cancel_registered_guest_wait(int pid)
{
    NativeGuestWait *wait;

    for (wait = native_guest_waits; wait; wait = wait->next)
    {
        if (wait->pid == pid)
        {
            int cancelled = cancel_guest_wait(wait);
            fvm_port_sync_signal_all(channel_queue_mutex);
            return cancelled;
        }
    }
    return -1;
}


/* A NativeWorld owner can also cancel a scheduler-managed wait. */
int dvm_runtime_cancel_guest_wait(int pid)
{
    int cancelled;

    if (!channel_queue_mutex || pid <= 0)
    {
        return 0;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    cancelled = cancel_registered_guest_wait(pid);
    fvm_port_sync_unlock(channel_queue_mutex);
    return cancelled < 0 ? cancel_scheduler_guest_wait(pid) : cancelled;
}


/* Request cancellation from an unbound host thread. Once a native wait is
 * registered, this request belongs to that continuation even if it cannot
 * be cancelled. Otherwise queue the PID for the scheduler or publication. */
int dvm_runtime_request_guest_cancel(int pid)
{
    int cancelled;

    if (!channel_queue_mutex || pid <= 0)
    {
        return 0;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    cancelled = cancel_registered_guest_wait(pid);
    if (cancelled >= 0)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return cancelled;
    }
    if (!cancel_requests_open || native_waits_stopping)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 0;
    }
    dvm_host_cancellations_post(&cancel_requests, pid);
    dvm_managed_workers_wake();
    fvm_port_sync_unlock(channel_queue_mutex);
    return 1;
}


/* Check the request queue without taking the scheduler mutex. */
static int guest_cancel_pending(void)
{
    int pending;

    fvm_port_sync_lock(channel_queue_mutex);
    pending = cancel_requests.count != 0;
    fvm_port_sync_unlock(channel_queue_mutex);
    return pending;
}


/* Recheck controller work after sampling the wake generation. A peer
 * may have completed a channel wait before that generation was sampled. */
static int dedicated_controller_pending(void)
{
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)initial_guest;
    return (resident &&
            (__atomic_load_n(&resident->queued, __ATOMIC_ACQUIRE) || dvm_messages_pending(resident->messages, 0, 0) ||
             dvm_messages_pending(resident->messages, 1, 0))) ||
           guest_cancel_pending() ||
           (!dvm_runqueue_ready() && !dvm_runqueue_inflight() && !dvm_runqueue_dispatch_pending());
}


/* At the final idle boundary, either keep the controller alive to drain
 * accepted requests or close admission before releasing the ledger lock. */
static int guest_cancel_pending_or_close(void)
{
    int pending;
    fvm_port_sync_lock(channel_queue_mutex);

    /* A dedicated owner can have work before its guest becomes runnable. */
    if (native_spawn)
    {
        if (__atomic_load_n(&resident_retirements, __ATOMIC_ACQUIRE))
        {
            fvm_port_sync_unlock(channel_queue_mutex);
            return 1;
        }
        for (Prog *guest = dvm_runtime_process_roots(); guest; guest = guest->next)
        {
            DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
            if (process->channel_claimed || __atomic_load_n(&process->startup_pending, __ATOMIC_ACQUIRE) ||
                (process->messages &&
                 (dvm_messages_pending(process->messages, 0, 0) || dvm_messages_pending(process->messages, 1, 0) ||
                  dvm_managed_workers_reply_in_transit(guest))))
            {
                fvm_port_sync_unlock(channel_queue_mutex);
                return 1;
            }
        }
    }
    /* Read the source of each handoff before the destination it publishes. */
    if (native_spawn &&
        (__atomic_load_n(&resident_wait_deliveries, __ATOMIC_ACQUIRE) || dvm_runqueue_dispatch_pending() ||
         __atomic_load_n(&resident_retirements, __ATOMIC_ACQUIRE) || dvm_runqueue_inflight() || dvm_runqueue_ready()))
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 1;
    }
    pending = cancel_requests.count != 0;
    if (!pending)
    {
        cancel_requests_open = 0;
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return pending;
}

#define DVM_CHILD_EXIT_WAIT (Pbroken + 2) /* Hosted scheduler state distinct from Bigtable waits. */


/* EN: Classify a live borrowed resident for a mailbox request. Caller protects
 * registration under channel_queue_mutex. Read only atomic state/retry values;
 * a later wait change is resolved by the owner, not by this admission hint.
 * RU: Проверить живой заимствованный process перед сообщением mailbox. Caller
 * защищает регистрацию под channel_queue_mutex. Читать лишь атомарные state/retry;
 * последующую смену ожидания разрешает владелец, а не эта предварительная проверка. */
static int resident_wait_request_supported(Prog *process)
{
    int state = dvm_runqueue_observed_state(process);
    return state == Psend || state == Precv || state == Palt || state == DVM_CHILD_EXIT_WAIT ||
           state == DVM_TABLE_WAIT ||
           (state == Pready &&
            __atomic_load_n(&((DVM_RuntimeProcess *)process)->table_retry_pending, __ATOMIC_ACQUIRE));
}
