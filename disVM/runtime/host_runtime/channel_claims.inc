/* Copyright (c) 2026 bhgv
 * EN: Channel completion claims and cancellation publication; included once by main.c.
 * RU: Захваты завершения каналов и публикация отмены; однократно подключается из main.c.
 */


typedef struct ChannelClaim
{
    Prog *peer;
    int (*validate)(void *); /* Optional queue-head check under the queue mutex. */
    int acquired;            /* Waiter was pinned and claimed. */
    void *request;           /* Borrowed completion callback argument. */
    Channel *channel;        /* Optional channel protected through the match. */
    DVM_ChannelMatch match;  /* Stack reservation under channel_queue_mutex. */
} ChannelClaim;


/* Acquire the completion hold while a scheduler or queue owner is live. */
static int acquire_channel_claim(void *argument)
{
    ChannelClaim *claim = argument;
    Prog *peer = claim->peer;
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
    if (claim->validate && !claim->validate(claim->request))
    {
        return 1;
    }
    DVM_NativeContext *dispatch = dvm_runqueue_dispatch_owner(peer);
    /* EN: ALT publishes its wait before its owner finishes saving registers.
     * A foreign matcher must retry until that turn has released the owner.
     * RU: ALT публикует ожидание до завершения сохранения регистров владельцем.
     * Чужой участник повторяет попытку после завершения этого вызова ALT. */
    if (native_spawn && claim->validate && peer->state == Palt && dispatch && dispatch != dvm_native_context_current())
    {
        return 1;
    }
    if (process->channel_claimed || (peer->state != Psend && peer->state != Precv && peer->state != Palt))
    {
        if (native_spawn && claim->validate)
        {
            return 1;
        }
        error("channel waiter cannot be claimed");
    }
    if (claim->channel && !dvm_channel_match_begin(&claim->match, claim->channel, dvm_native_context_current()))
    {
        return 1;
    }
    if (!dvm_runqueue_pin(peer))
    {
        if (claim->channel)
        {
            dvm_channel_match_end(&claim->match);
        }
        error("channel waiter cannot be pinned");
    }
    process->channel_claimed = claim;
    claim->acquired = 1;
    return 1;
}


/* Release the exact completion claim under the same mutex as acquisition.
 * peer remains pinned until after the claim is cleared. A mismatched identity
 * indicates a broken ownership invariant and cannot be recovered in the VM. */
static void release_channel_claim(Prog *peer, const void *owner, DVM_ChannelMatch *match)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
    int runnable;
    fvm_port_sync_lock(channel_queue_mutex);
    if (process->channel_claimed != owner)
    {
        abort();
    }
    runnable = __atomic_load_n(&process->queued, __ATOMIC_ACQUIRE);
    if (match)
    {
        dvm_channel_match_end(match);
        dvm_runtime_channel_buffer_notify();
    }
    dvm_runqueue_unpin(peer);
    process->channel_claimed = nil;
    if (runnable)
    {
        dvm_managed_workers_wake_guest(peer);
    }
    fvm_port_sync_unlock(channel_queue_mutex);
}


/* Finish outside the mutex. Cancellation and wake can enter the queue guard. */
static void finish_channel_claim(ChannelClaim *claim, int (*match)(void *))
{
    Prog *peer = claim->peer;
    int completed = kprotect(match, claim->request);
    release_channel_claim(peer, claim, claim->channel ? &claim->match : NULL);
    if (!completed)
    {
        krethrow();
    }
}


/* External cancellation borrows the scheduler-owned waiter under NativeWorld. */
void dvm_runtime_channel_with_claim(Prog *peer, int (*match)(void *), void *request)
{
    ChannelClaim claim = {peer, nil, 0, request, NULL, {0}};
    dvm_runtime_channel_queue_guard(acquire_channel_claim, &claim);
    finish_channel_claim(&claim, match);
}


/* Check the queue head and pin it in the same mutex acquisition. */
int dvm_runtime_channel_with_queue_claim(
    Prog *peer, int (*validate)(void *), int (*match)(void *), void *request, Channel *channel)
{
    ChannelClaim claim = {peer, validate, 0, request, channel, {0}};
    dvm_runtime_channel_queue_guard(acquire_channel_claim, &claim);
    if (claim.acquired)
    {
        finish_channel_claim(&claim, match);
    }
    return claim.acquired;
}


/* Complete one detached wait under NativeWorld. Scheduler admission is the
 * state transition. Refusal leaves wait fields and any pending kill intact. */
void dvm_runtime_channel_wake(Prog *peer, char *cause)
{
    if (peer->state != Psend && peer->state != Precv && peer->state != Palt)
    {
        error("invalid channel waiter state");
    }
    if (!dvm_runqueue_complete_channel_wait(peer, cause))
    {
        error("invalid runnable process admission");
    }
}


extern void dvm_runtime_alt_preflight(Alt *alt);


/* EN: Wake a detached borrowed waiter while the cancellation result is scoped.
 * RU: Пробудить снятый с очереди процесс внутри области результата отмены. */
static int wake_cancelled_channel_wait(void *argument)
{
    Prog *peer = argument;
    dvm_runtime_channel_wake(peer, peer->kill ? nil : "channel wait cancelled");
    return 1;
}


/* Remove this guest's registrations under its completion claim. A pending
 * exception survives cancellation; a detached waiter is made runnable once. */
static int cancel_parked_channel_wait(void *argument)
{
    Prog *peer = argument;

    if (peer->state == Palt)
    {
        dvm_runtime_alt_preflight(peer->R.s);
        altdone(peer->R.s, peer, nil, -1);
    }
    else
    {
        Channel *channel = peer->chan;
        Progq **queue;

        if (!channel || channel == H)
        {
            error(exHeap);
        }
        queue = peer->state == Psend ? &channel->send : &channel->recv;
        cqdelp(queue, peer);
    }
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
    if (!peer->kill && process->messages && dvm_messages_pending(process->messages, 0, DVM_PROG_MESSAGE_CHANNEL_CLOSE))
    {
        peer->kill = "channel hangup";
    }
    process->cancel_delivering = 1;
    int completed = kprotect(wake_cancelled_channel_wait, peer);
    process->cancel_delivering = 0;
    if (!completed)
    {
        krethrow();
    }
    return 1;
}


/* EN: Check a borrowed resident's wait while holding the channel queue mutex.
 * A concurrent match may already own or complete it; false leaves it unchanged.
 * RU: Проверить ожидание заимствованного процесса под мьютексом очереди каналов.
 * Соперничающая передача могла уже захватить/завершить его; ложь ничего не меняет. */
static int cancellable_channel_wait(void *argument)
{
    Prog *peer = argument;
    return peer && (peer->state == Psend || peer->state == Precv || peer->state == Palt) &&
           !((DVM_RuntimeProcess *)peer)->channel_claimed && !dvm_runqueue_dispatch_owner(peer);
}


/* EN: Atomically validate and claim a borrowed channel wait for cancellation.
 * Return zero when a real transfer won; preserve its result and pending messages.
 * RU: Атомарно проверить и захватить заимствованное ожидание для отмены.
 * Вернуть ноль, если передача выиграла; сохранить её результат и сообщения. */
static int cancel_parked_guest(Prog *peer)
{
    if (!peer)
    {
        return 0;
    }
    return dvm_runtime_channel_with_queue_claim(peer, cancellable_channel_wait, cancel_parked_channel_wait, peer, NULL);
}


static int cancel_child_exit_wait(Prog *process);


/* Cancel a scheduler-managed wait by PID. The caller must
 * own NativeWorld. An unbound, suspended, or already completed guest is left
 * alone. The public entry checks the native ledger before this fallback. */
static int cancel_scheduler_guest_wait(int pid)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    Prog *peer;

    if (native_spawn || pid <= 0 || !dvm_native_owner_world(owner) || !dvm_native_owner_statistics(owner, &state) ||
        state.phase != DVM_HEAP_ACTIVE)
    {
        return 0;
    }
    for (peer = dvm_runtime_process_roots(); peer && peer->pid != pid; peer = peer->next)
    {
    }
    return peer && !dvm_runqueue_dispatch_owner(peer) &&
           (cancel_parked_guest(peer) || dvm_bigtable_cancel_wait(peer) || cancel_child_exit_wait(peer));
}


/* EN: Borrow a direct child. Dedicated callers hold channel_queue_mutex for
 * the entire lookup/use; the single-threaded reference owns its managed turn.
 * RU: Заимствовать прямого ребёнка. Выделенные потоки держат channel_queue_mutex
 * на всё время поиска/использования; однопоточный эталон владеет своим вызовом. */
static Prog *find_child(Prog *caller, int pid)
{
    Prog *child;

    for (child = dvm_runtime_process_roots(); child && child->pid != pid; child = child->next)
    {
    }
    return child && ((DVM_RuntimeProcess *)child)->parent_pid == caller->pid ? child : nil;
}


/* Authorize the hosted builtin against immutable spawning identity. */
int dvm_runtime_cancel_child_wait(Prog *caller, int pid)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)caller;
    if (process->cancel_ready_pid == pid)
    {
        process->cancel_ready_pid = 0;
        return process->cancel_ready_result;
    }
    if (native_spawn)
    {
        fvm_port_sync_lock(channel_queue_mutex);
        int result = Processctl_Denied;
        if (find_child(caller, pid))
        {
            result = cancel_registered_guest_wait(pid) == 1 ? Processctl_Cancelled : Processctl_NotWaiting;
        }
        fvm_port_sync_unlock(channel_queue_mutex);
        return result;
    }
    Prog *child = find_child(caller, pid);
    if (!child)
    {
        return Processctl_Denied;
    }
    return cancel_parked_guest(child) || dvm_bigtable_cancel_wait(child) || cancel_child_exit_wait(child) ||
                   dvm_runtime_cancel_guest_wait(pid)
               ? Processctl_Cancelled
               : Processctl_NotWaiting;
}


/* EN: Post a cancellation for the child's supported wait and park caller.
 * RU: Отправить отмену поддерживаемого ожидания потомка и припарковать caller. */
int dvm_runtime_prepare_cancelwait(Prog *caller, int pid)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)caller;
    if (!native_spawn || process->cancel_ready_pid == pid)
    {
        return 1;
    }


    fvm_port_sync_lock(channel_queue_mutex);
    Prog *child = find_child(caller, pid);
    if (!child || !resident_wait_request_supported(child))
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 1;
    }
    DVM_RuntimeProcess *target = (DVM_RuntimeProcess *)child;
    if (target->channel_claimed)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 1;
    }
    DVM_OwnerMessage *request = &target->cancel_wait;
    process->exit_wait_pid = pid;
    process->cancel_waiting = 1;
    if (!dvm_runqueue_pop_expected(caller, DVM_CHILD_EXIT_WAIT))
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        error("invalid runnable process removal");
    }
    /* The child may already be polling its mailbox. Publish the caller's
     * reply destination and wait state before making the request visible. */
    if (!dvm_messages_send_once(guest_messages, (uintptr_t)pid, request))
    {
        process->exit_wait_pid = 0;
        process->cancel_waiting = 0;
        addrun(caller);
        fvm_port_sync_unlock(channel_queue_mutex);
        error("cannot post wait cancellation");
    }
    DVM_R.PC--;
    DVM_R.IC = 1;
    DVM_R.t = 1;
    dvm_managed_workers_wake_guest(child);
    fvm_port_sync_unlock(channel_queue_mutex);
    return 0;
}
