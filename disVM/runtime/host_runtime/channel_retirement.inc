/* Copyright (c) 2026 bhgv
 * EN: Detached channel waits and native resource finalization; included once by main.c.
 * RU: Отсоединённые ожидания каналов и завершение нативных ресурсов; однократно подключается из main.c.
 */


/* [GC:] EN: Finish channel refill after ALT cancellation. peer/channel remain
 * independently live; refill/request and committed describe the staged transfer.
 * Preserve the first VM error; wake peer after attempting channel-owner release.
 * RU: Завершить refill channel после отмены ALT. peer/channel живы независимо;
 * refill/request и committed описывают передачу. Сохранить первую VM-ошибку,
 * разбудить peer после попытки снятия владельца канала. */
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


/* [GC:] EN: Wake borrowed argument's detached Prog while its scheduler pins
 * remain. Return one or raise a VM error for foreign C waits; caller catches it.
 * No foreign Prog is executed by this thread.
 * RU: Разбудить отсоединённый Prog argument при живых scheduler-pin. Вернуть
 * один либо VM-ошибку для чужого C-ожидания; вызывающий её перехватывает.
 * Чужой Prog этим потоком не исполняется. */
static int wake_closed_channel(void *argument)
{
    Prog *process = argument;
    switch (dvm_runqueue_observed_state(process))
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


/* [GC:] EN: Classify borrowed peer with lifetime protected by its queue or pin.
 * Read one published state: duplicate ALT entries may outlive
 * their first completion while the owner advances to a table wait.
 * Return whether its state uses hosted completion.
 * RU: Классифицировать peer, чья жизнь защищена очередью либо pin.
 * Читать публикацию state: повторные ALT-узлы могут пережить первое завершение
 * и переход владельца к ожиданию таблицы. Вернуть признак hosted-завершения. */
static int hosted_channel_waiter(Prog *peer)
{
    int state = peer ? dvm_runqueue_observed_state(peer) : Pready;
    return peer && (state == Psend || state == Precv || state == Palt);
}


/* [GC:] EN: Validate borrowed entry chain under the queue mutex before any
 * mutation. Raise a VM error for conflicting claims or invalid pin capacity.
 * RU: Под мьютексом проверить цепочку entry до изменений. Конфликт захвата
 * или неверная ёмкость pin вызывает VM-ошибку. */
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


/* [GC:] EN: Pin eligible entry peers for owner under the queue mutex after
 * complete preflight; duplicate ALT nodes share one claim. Corruption aborts.
 * RU: После полной проверки удержать процессы entry для owner под мьютексом;
 * повторные ALT-записи имеют один захват. Повреждение вызывает abort. */
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


/* [GC:] EN: Consume detached entry nodes and owner claims outside the queue
 * mutex. Release every scheduler pin; first_error/failed retain the first caught
 * wake error. Duplicate ALT registrations consume a claim only once.
 * RU: Вне мьютекса снять узлы entry и захваты owner, освободить все scheduler-pin.
 * first_error/failed сохраняют первую ошибку пробуждения. Повторные регистрации
 * ALT потребляют захват лишь один раз. */
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
        if (claimed || (peer && dvm_runqueue_observed_state(peer) == Prelease))
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


/* [GC:] EN: Borrow queue owners until guarded detach transfers their chains,
 * buffer and mapped descriptor. Caller consumes detached outputs after unlocking.
 * RU: Заимствовать владельцев очередей до передачи цепочек, буфера и дескриптора
 * под guard. После unlock вызывающий потребляет отсоединённые выходы. */
typedef struct ClosedQueues
{
    Channel *channel;   /* Whole-channel finalization only. */
    Progq **send_owner; /* Borrowed first queue owner until detach. */
    Progq **recv_owner; /* NULL for the single-queue entry point. */
    Progq *send;        /* Detached first chain owned by this operation. */
    Progq *recv;        /* Detached second chain, if present. */
    Array *buffer;      /* Transferred buffer owner after successful detach. */
    Type *mapped;       /* Transferred mover descriptor owner, if present. */
} ClosedQueues;


/* [GC:] EN: Validate borrowed ClosedQueues argument under the queue mutex,
 * then transfer both queue owners and optional channel resources. Return one;
 * VM refusal precedes mutation. Later wakeups may remove sibling registrations.
 * RU: Под мьютексом проверить ClosedQueues argument, затем передать очереди
 * и ресурсы канала. Вернуть один; VM-отказ предшествует изменениям. Позднее
 * пробуждение может снимать соседние регистрации. */
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


/* [GC:] EN: Detach the borrowed queue owner under its mutex, then consume
 * nodes/pins and wake outside it. Propagate the first VM error after cleanup.
 * Caller keeps queue storage live and supplies destructor admission.
 * RU: Под мьютексом отсоединить queue, затем снять узлы/pin и разбудить снаружи.
 * После очистки передать первую VM-ошибку. Вызывающий удерживает память очереди и допуск. */
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


/* [GC:] EN: Snapshot borrowed channel quietness under the queue mutex.
 * Empty queues may own distinct sentinels. Return boolean without mutation.
 * RU: Под мьютексом проверить спокойное состояние channel. Пустые очереди
 * могут владеть разными sentinel. Вернуть bool без изменений. */
static int channel_quiet(Channel *channel)
{
    Progq *send = channel->send;
    Progq *recv = channel->recv;
    return !channel->aux && (!send || (!send->prog && !send->next && send != recv)) &&
           (!recv || (!recv->prog && !recv->next)) && !dvm_runtime_channel_buffer_busy(channel);
}


/* [GC:] EN: Try a queue-locked snapshot of independently live channel. The
 * collector may hold cell guards/reservations while ALT holds the queue mutex
 * and waits for cells: never wait here. Return -1 on contention (retry), zero
 * for nonquiet, one for quiet; never a root or last-owner claim.
 * RU: Попробовать снимок живого channel под мьютексом очереди. GC может
 * держать cell guards/reservations, пока ALT держит очередь и ждёт cells:
 * здесь нельзя ждать. Вернуть -1 при конфликте (повторить), ноль при активности,
 * один при покое. Результат не является корнем либо захватом владельца. */
int dvm_runtime_channel_is_quiet(Channel *channel)
{
    if (!fvm_port_sync_try_lock(channel_queue_mutex))
    {
        return -1;
    }
    int quiet = channel_quiet(channel);
    fvm_port_sync_unlock(channel_queue_mutex);
    return quiet;
}


/* [GC:] EN: Classify committed waits on borrowed channel under the queue mutex.
 * Return one for hosted waiters, zero when none exist, minus one for unsupported
 * transport/native waiters. No queue mutation, retain or message occurs.
 * RU: Классифицировать ожидания заимствованного channel под мьютексом очереди.
 * Один — hosted-ожидания, ноль — пусто, минус один — транспорт/чужие native-
 * ожидания. Очереди, ссылки и почтовые ящики не меняются. */
static int channel_waiter_kind(Channel *channel)
{
    if (!native_spawn || channel->aux)
    {
        return -1;
    }
    Progq *queues[] = {channel->send, channel->recv};
    int found = 0;
    for (size_t index = 0; index < sizeof(queues) / sizeof(*queues); index++)
    {
        for (Progq *entry = queues[index]; entry; entry = entry->next)
        {
            if (entry->prog)
            {
                if (!hosted_channel_waiter(entry->prog))
                {
                    return -1;
                }
                found = 1;
            }
        }
    }
    return found;
}


/* [GC:] EN: Post close messages to hosted owners waiting on borrowed channel.
 * Unsupported/empty queues return zero unchanged. The queue mutex protects
 * process identities through send/wake; no foreign Prog executes on this thread.
 * RU: Передать закрытие hosted-владельцам, ожидающим заимствованный channel.
 * Чужие/пустые очереди возвращают ноль без изменений. Мьютекс защищает личности
 * процессов до send/wake; чужой Prog не исполняется этим потоком. */
int dvm_runtime_channel_post_close(Channel *channel)
{
    int posted = 0;
    fvm_port_sync_lock(channel_queue_mutex);
    if (channel_waiter_kind(channel) > 0)
    {
        Progq *queues[] = {channel->send, channel->recv};
        for (size_t index = 0; index < sizeof(queues) / sizeof(*queues); index++)
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


/* [GC:] EN: Try the callback-free claim after the reader cutoff. heap remains
 * live; buffer is a captured H/array identity and is not dereferenced here.
 * mapped receives an owned mover descriptor only on
 * success. already_claimed selects graph count zero versus a last-count CAS.
 * Under the same queue mutex preserve transient refusal in optional retry:
 * changed buffer, active transfer, hosted waits or a lost quiet claim. Transport
 * and foreign-waiter refusal remain hard. Failed calls do not detach ownership.
 * RU: После эпохи читателей попробовать захват без callbacks. heap жив;
 * buffer — сохранённая H/array-идентичность, здесь не разыменовывается.
 * mapped получает дескриптор лишь при успехе. already_claimed выбирает нулевой
 * счётчик графа либо последний CAS. Под тем же мьютексом сохранить в retry
 * временную причину отказа: смену буфера, передачу, hosted-ожидания или отказ
 * спокойного захвата. Транспорт/чужие ожидания остаются жёстким отказом.
 * Неудача не отсоединяет владение. */
int dvm_runtime_channel_claim_observed(Heap *heap, Array *buffer, Type **mapped, int already_claimed, int *retry)
{
    Channel *channel = H2D(Channel *, heap);
    fvm_port_sync_lock(channel_queue_mutex);
    Progq *send = channel->send;
    Progq *recv = channel->recv;
    int quiet = channel_quiet(channel);
    int claimed = channel->buf == buffer && quiet &&
                  (already_claimed ? dvm_refcount_load_u(&heap->ref) == 0 : dvm_refcount_claim_last_u(&heap->ref));
    if (!claimed && retry && !channel->aux)
    {
        *retry |= channel->buf != buffer || quiet || dvm_runtime_channel_buffer_busy(channel) ||
                  channel_waiter_kind(channel) > 0;
    }
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


/* [GC:] EN: Compatibility claim for graph finalization and existing callers.
 * Forward borrowed heap/buffer and owned mapped output unchanged; no retry
 * output is requested. already_claimed keeps its original count-zero meaning.
 * RU: Совместимый захват для графов и прежних вызовов. Передать heap/buffer и
 * выход mapped без изменения, без выхода retry. already_claimed сохраняет
 * прежний смысл нулевого счётчика. */
int dvm_runtime_channel_claim_quiet(Heap *heap, Array *buffer, Type **mapped, int already_claimed)
{
    return dvm_runtime_channel_claim_observed(heap, buffer, mapped, already_claimed, NULL);
}


/* [GC:] EN: Finalize borrowed channel under caller-supplied destructor
 * admission. Guarded detach transfers queues/buffer/mover ownership; wake and
 * release after unlocking. swept suppresses ordinary buffer destruction. Retain
 * the first VM error until cleanup finishes; no arbitrary shared callback is authorized.
 * RU: Завершить channel под допуском деструктора вызывающего. Guard передаёт
 * очереди/буфер/mover; пробуждение и снятие ссылок идут после unlock. swept
 * подавляет обычное разрушение буфера. Первую VM-ошибку передать после очистки;
 * произвольный shared-callback не разрешается. */
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


/* [GC:] EN: Legacy Sys.FD callback for independently live heap; swept is unused.
 * Close only hosted descriptors 3..31 through platform ports. This is not a
 * declaration of arbitrary-callback safety under shared admission.
 * RU: Старый callback Sys.FD живого heap; swept не используется. Закрыть лишь
 * hosted-дескрипторы 3..31 через порты. Безопасность произвольных shared-callback
 * этим не объявляется. */
static void free_fd(Heap *heap, int swept)
{
    Sys_FD *fd = H2D(Sys_FD *, heap);
    USED(swept);
    if (fd->fd >= 3 && fd->fd < 32)
    {
        kclose(fd->fd);
    }
}
