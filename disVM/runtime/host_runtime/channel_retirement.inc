/* Copyright (c) 2026 bhgv
 * EN: Detached channel waits and native resource finalization; included once by main.c.
 * RU: Отсоединённые ожидания каналов и завершение нативных ресурсов; однократно подключается из main.c.
 */


/* Finish a buffered rendezvous under NativeWorld after ALT cancellation.
 * A refill error is primary. A later release error still reaches the receiver
 * but the sender succeeds once its payload is published. */
void dvm_runtime_channel_finish_refill(
    Channel *channel, Prog *peer, int (*refill)(void *), void *request, const int *committed)
{
    char first_error[ERRMAX];
    int transferred = kprotect(refill, request);
    int released;
    if (!transferred)
    {
        memmove(first_error, owner_error(), sizeof(first_error));
    }
    released = kprotect(release_refill_channel, channel);
    dvm_runtime_channel_wake(peer, transferred || *committed ? nil : "channel transfer failed");
    if (!transferred)
    {
        error(first_error);
    }
    if (!released)
    {
        krethrow();
    }
}


extern void dvm_runtime_altgone_claimed(Prog *peer);


/* Wake one detached waiter while the scheduler still owns its Prog. */
static int wake_closed_channel(void *argument)
{
    Prog *process = argument;
    switch (process->state)
    {
    case Psend:
    case Precv:
        dvm_runtime_channel_wake(process, "channel hangup");
        break;
    case Palt:
        dvm_runtime_altgone_claimed(process);
        break;
    case Prelease:
        error("foreign C channel waiter is not supported by this host");
        break;
    default:
        /* A previous registration may already have woken the same ALT. */
        break;
    }
    return 1;
}


extern Progq *dvm_runtime_channel_close_preflight(Progq **queue);


/* Test whether this queue entry participates in hosted channel completion. */
static int hosted_channel_waiter(Prog *peer)
{
    return peer && (peer->state == Psend || peer->state == Precv || peer->state == Palt);
}


/* Refuse another owner's claim before changing either queue. */
static void check_close_claims(Progq *entry)
{
    for (; entry; entry = entry->next)
    {
        Prog *peer = entry->prog;
        if (hosted_channel_waiter(peer) &&
            (((DVM_RuntimeProcess *)peer)->channel_claimed || dvm_runqueue_hold_count(peer) == INT_MAX ||
             dvm_runqueue_hold_count(peer) < 0))
        {
            error("channel waiter cannot be claimed");
        }
    }
}


/* Reserve all eligible peers with one identity, including duplicate ALT nodes. */
static void claim_close_waiters(Progq *entry, const void *owner)
{
    for (; entry; entry = entry->next)
    {
        Prog *peer = entry->prog;
        if (hosted_channel_waiter(peer) && !((DVM_RuntimeProcess *)peer)->channel_claimed)
        { /* The complete preflight checked capacity before any mutation. */
            if (!dvm_runqueue_pin(peer))
            {
                abort();
            }
            ((DVM_RuntimeProcess *)peer)->channel_claimed = owner;
        }
    }
}


/* Drain an exclusively detached chain, recording the first wake failure.
 * Only the first registration consumes the claim, even on a wake error. */
static void drain_closed_queue(Progq *entry, const void *owner, char *first_error, int *failed)
{
    while (entry)
    {
        Progq *next = entry->next;
        Prog *peer = entry->prog;
        DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
        int claimed = hosted_channel_waiter(peer) && process->channel_claimed == owner;
        free(entry);
        entry = next;
        if (claimed || (peer && peer->state == Prelease))
        {
            int completed = kprotect(wake_closed_channel, peer);
            if (claimed)
            {
                release_channel_claim(peer, owner, NULL);
            }
            if (!completed && !*failed)
            {
                memmove(first_error, owner_error(), ERRMAX);
                *failed = 1;
            }
        }
        if (peer)
        {
            dvm_runqueue_unpin(peer);
        }
    }
}


typedef struct ClosedQueues
{
    Channel *channel;   /* Whole-channel finalization only. */
    Progq **send_owner;
    Progq **recv_owner; /* NULL for the single-queue entry point. */
    Progq *send;
    Progq *recv;
    Array *buffer; /* Transferred buffer owner after successful detach. */
    Type *mapped;  /* Transferred mover descriptor owner, if present. */
} ClosedQueues;


/* Validate and detach both owners while the queue mutex is held.
 * Wakeups run later and may themselves remove sibling registrations. */
static int detach_closed_queues(void *argument)
{
    ClosedQueues *queues = argument;
    Progq *send = *queues->send_owner;
    Progq *recv = queues->recv_owner ? *queues->recv_owner : nil;
    Progq *send_tail = send ? dvm_runtime_channel_close_preflight(queues->send_owner) : nil;
    Progq *recv_tail = recv ? dvm_runtime_channel_close_preflight(queues->recv_owner) : nil;
    if (queues->channel && dvm_runtime_channel_buffer_busy(queues->channel))
    {
        error("channel buffer transfer already active");
    }
    if (send_tail && send_tail == recv_tail)
    {
        error(exHeap);
    }
    check_close_claims(send);
    check_close_claims(recv);
    claim_close_waiters(send, queues);
    claim_close_waiters(recv, queues);
    queues->send = send;
    queues->recv = recv;
    *queues->send_owner = nil;
    if (queues->recv_owner)
    {
        *queues->recv_owner = nil;
    }
    if (queues->channel)
    {
        Channel *channel = queues->channel;
        queues->buffer = channel->buf;
        queues->mapped = channel->mover == movtmp ? channel->mid.t : nil;
        channel->buf = H;
        channel->mover = nil;
    }
    return 1;
}


/* Detach under the queue mutex and wake after unlocking. */
void killcomm(Progq **queue)
{
    ClosedQueues queues = {nil, queue, nil, nil, nil, H, nil};
    char first_error[ERRMAX];
    int failed = 0;
    dvm_runtime_channel_queue_guard(detach_closed_queues, &queues);
    drain_closed_queue(queues.send, &queues, first_error, &failed);
    if (failed)
    {
        error(first_error);
    }
}


/* Caller holds the channel queue mutex. Empty queues may own sentinels. */
static int channel_quiet(Channel *channel)
{
    Progq *send = channel->send;
    Progq *recv = channel->recv;
    return !channel->aux && (!send || (!send->prog && !send->next && send != recv)) &&
           (!recv || (!recv->prog && !recv->next)) && !dvm_runtime_channel_buffer_busy(channel);
}


int dvm_runtime_channel_is_quiet(Channel *channel)
{
    fvm_port_sync_lock(channel_queue_mutex);
    int quiet = channel_quiet(channel);
    fvm_port_sync_unlock(channel_queue_mutex);
    return quiet;
}


/* EN: Post a close message to owners of the waits registered on channel.
 * Return zero for waiters outside dedicated mode or transport-owned channels.
 * RU: Передать закрытие владельцам ожиданий на channel. Вернуть ноль для
 * ожиданий вне выделенного режима или каналов под управлением транспорта. */
int dvm_runtime_channel_post_close(Channel *channel)
{
    int posted = 0;
    fvm_port_sync_lock(channel_queue_mutex);
    if (native_spawn && !channel->aux)
    {
        Progq *queues[] = {channel->send, channel->recv};
        int supported = 1;
        for (size_t index = 0; supported && index < sizeof(queues) / sizeof(*queues); index++)
        {
            for (Progq *entry = queues[index]; entry; entry = entry->next)
            {
                Prog *peer = entry->prog;
                if (peer && !hosted_channel_waiter(peer))
                {
                    supported = 0;
                    break;
                }
            }
        }
        for (size_t index = 0; supported && index < sizeof(queues) / sizeof(*queues); index++)
        {
            for (Progq *entry = queues[index]; entry; entry = entry->next)
            {
                Prog *peer = entry->prog;
                if (peer)
                {
                    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
                    if (dvm_messages_send_once(guest_messages, (uintptr_t)peer->pid, &process->channel_close) == 1)
                    {
                        dvm_managed_workers_wake_guest(peer);
                    }
                    posted = 1;
                }
            }
        }
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return posted;
}


/* Try the callback-free last-owner path after the reader epoch. */
int dvm_runtime_channel_claim_quiet(Heap *heap, Array *buffer, Type **mapped, int already_claimed)
{
    Channel *channel = H2D(Channel *, heap);
    fvm_port_sync_lock(channel_queue_mutex);
    Progq *send = channel->send;
    Progq *recv = channel->recv;
    int claimed = channel->buf == buffer && channel_quiet(channel) &&
                  (already_claimed ? dvm_refcount_load_u(&heap->ref) == 0 : dvm_refcount_claim_last_u(&heap->ref));
    if (claimed)
    {
        *mapped = channel->mover == movtmp ? channel->mid.t : nil;
        channel->send = nil;
        channel->recv = nil;
        channel->buf = H;
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    if (claimed)
    {
        free(send);
        free(recv);
    }
    return claimed;
}


/* Finalize both queue owners together. NativeWorld keeps the channel and
 * registered processes live through resource release. */
void dvm_runtime_channel_finalize(Channel *channel, int swept)
{
    ClosedQueues queues = {channel, &channel->send, &channel->recv, nil, nil, H, nil};
    char first_error[ERRMAX];
    int failed = 0;
    dvm_runtime_channel_queue_guard(detach_closed_queues, &queues);
    drain_closed_queue(queues.send, &queues, first_error, &failed);
    drain_closed_queue(queues.recv, &queues, first_error, &failed);
    if (queues.mapped)
    {
        freetype(queues.mapped);
    }
    if (!swept && queues.buffer != H && !kprotect(release_refill_channel, queues.buffer) && !failed)
    {
        memmove(first_error, owner_error(), ERRMAX);
        failed = 1;
    }
    if (failed)
    {
        error(first_error);
    }
}


/* Retained runt.c provides string services; hosted FD services use platform ports. */
static void free_fd(Heap *heap, int swept)
{
    Sys_FD *fd = H2D(Sys_FD *, heap);
    USED(swept);
    if (fd->fd >= 3 && fd->fd < 32)
    {
        kclose(fd->fd);
    }
}
