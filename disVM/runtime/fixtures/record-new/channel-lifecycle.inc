/* Copyright (c) 2026 bhgv
 * EN: Channel creation, detached drain and finalization probes.
 * RU: Проверки создания, снятия очередей и завершения каналов.
 * Included once by test_record_new.c in declaration order.
 */

static int channel_events; /* Completed candidate notifications. */
static int channel_failure; /* Raise in the buffer notification. */
static Channel *observed_channel; /* Borrowed during the monitor only. */


/* Check that the channel is rooted before either monitor notification. */
static void channel_monitor(int event, void *value, ulong bytes)
{
    Heap *header = value;
    if (event)
    {
        return;
    }
    channel_events++;
    if (header->t == &Tchannel)
    {
        Array *buffer;
        void *head;
        observed_channel = H2D(Channel *, header);
        buffer = observed_channel->buf;
        check(bytes == sizeof(Channel) && dvm_runtime_root_count(header) == 1 &&
            observed_channel->send && observed_channel->recv &&
            observed_channel->mover == movtmp && observed_channel->mid.t == record &&
            buffer != H && buffer->len == 3 && buffer->t == record, "complete rooted channel");
        memmove(&head, buffer->data, sizeof(head));
        check(head == H, "channel buffer mapped initialization");
    }
    else
    {
        check(header->t == &Tarray && observed_channel->buf == H2D(Array *, header) &&
            dvm_runtime_root_count(D2H(observed_channel)) == 1, "buffer monitor has channel root");
        if (channel_failure)
        {
            error("channel buffer monitor primary");
        }
    }
}


/* Invoke the mapped constructor in an error checkpoint. */
static int construct_channel(void *unused)
{
    (void)unused;
    check(dvm_runtime_channel_new(destination, record, 3, movtmp, 0) == observed_channel,
        "committed channel identity");
    return 1;
}


/* A buffer-monitor error must preserve the previously published channel. */
static void channel_case(void)
{
    Channel *previous;
    Channel *current;
    heapmonitor = channel_monitor;
    check(kprotect(construct_channel, NULL) && channel_events == 2, "channel publication");
    memmove(&previous, destination, sizeof(previous));
    previous_owner = D2H(previous);
    reject_root_type = &Tchannel;
    check(!kprotect(construct_channel, NULL) && root_refusals == 2 &&
        channel_events == 2, "channel root refusal precedes monitor");
    reject_root_type = NULL;
    previous_owner = NULL;
    channel_failure = 1;
    check(!kprotect(construct_channel, NULL) && channel_events == 4 &&
        strstr(getup()->env->errstr, "channel buffer monitor primary"), "channel error preserved");
    memmove(&current, destination, sizeof(current));
    check(previous == current, "channel error preserves owner");
    heapmonitor = NULL;
    observed_channel = NULL;
    puts("PASS guarded channels");
}


/* Close through the hosted callback using the actual VM error stack. */
static int close_waiters(void *argument)
{
    killcomm(argument);
    return 1;
}


typedef struct QueueAddProbe
{
    Progq **queue;
    Prog *peer;
} QueueAddProbe;


/* Enter the real guarded registration boundary under a VM checkpoint. */
static int probe_queue_add(void *argument)
{
    QueueAddProbe *probe = argument;
    cqadd(probe->queue, probe->peer);
    return 1;
}


/* One foreign waiter raises. ALT cancellation must still remove a sibling
 * registration and wake exactly once, followed by the ordinary sender. */
static void channel_drain_case(void)
{
    DVM_RuntimeProcess foreign = {0}, alternative = {0}, sender = {0};
    Channel *closed = H, *sibling = H;
    Alt *guards = calloc(1, offsetof(Alt, ac) + 2*sizeof(Altc));
    Prog *prior = dvm_runqueue_head();
    check(guards && prior && !prior->link && dvm_runqueue_pop(Psend) == prior,
        "temporarily withdraw the single startup process");
    closed = dvm_runtime_channel_new(&closed, &Tword, 0, movtmp, 0);
    sibling = dvm_runtime_channel_new(&sibling, &Tword, 0, movtmp, 0);
    dvm_runqueue_set_state(&foreign.process, Prelease);
    dvm_runqueue_set_state(&alternative.process, Palt);
    dvm_runqueue_set_state(&sender.process, Psend);
    guards->nsend = 2;
    guards->ac[0].c = closed;
    guards->ac[1].c = sibling;
    alternative.process.R.s = guards;
    cqadd(&closed->send, &foreign.process);
    {
        QueueAddProbe probe = {&closed->send, &alternative.process};
        closed->send->next = closed->send;
        check(!kprotect(probe_queue_add, &probe) && !alternative.lifetime_holds,
            "malformed registration leaves its hold unpublished");
        closed->send->next = NULL;
        /* The next registration below must acquire the same mutex. */
    }
    cqadd(&closed->send, &alternative.process);
    cqadd(&closed->send, &alternative.process);
    cqadd(&closed->send, &sender.process);
    cqadd(&sibling->send, &alternative.process);
    check(alternative.lifetime_holds == 3 && sender.lifetime_holds == 1,
        "each published queue node owns a Prog hold");
    {
        Progq *head = closed->send;
        Progq *next = head->next;
        head->next = head;
        check(!kprotect(close_waiters, &closed->send) &&
            !strcmp(getup()->env->errstr, exHeap) && closed->send == head &&
            head->next == head && !dvm_runqueue_head(),
            "close rejects a cycle in its own queue before detachment");
        head->next = next;
    }
    sibling->send->next = sibling->send;
    check(!kprotect(close_waiters, &closed->send) &&
        !strcmp(getup()->env->errstr, exHeap) &&
        closed->send->prog == &foreign.process &&
        closed->send->next->prog == &alternative.process &&
        sibling->send->prog == &alternative.process &&
        alternative.process.state == Palt && !dvm_runqueue_head(),
        "close refusal leaves queues and wait state intact");
    sibling->send->next = NULL;
    check(!kprotect(close_waiters, &closed->send) &&
        strstr(getup()->env->errstr, "foreign C channel waiter"),
        "drain preserves first wake error");
    check(!alternative.channel_claimed && !sender.channel_claimed &&
        !foreign.lifetime_holds && !alternative.lifetime_holds && !sender.lifetime_holds &&
        !closed->send && sibling->send && !sibling->send->prog &&
        dvm_runqueue_pop(Psend) == &alternative.process &&
        dvm_runqueue_pop(Psend) == &sender.process && !dvm_runqueue_head(),
        "detached drain cancels siblings and wakes unique peers in order");
    check(dvm_runqueue_append(prior), "restore startup process");
    destroy(sibling);
    destroy(closed);
    free(guards);
    puts("PASS guarded channel drain");
}


/* Exercise the actual hosted Heap finalizer without retiring the test header. */
static int finalize_channel(void *argument)
{
    extern void freechan(Heap *, int);
    freechan(D2H(argument), 0);
    return 1;
}


/* Submit a second completion to the real shared wake entry. */
static int repeat_channel_wake(void *argument)
{
    extern void dvm_runtime_channel_wake(Prog *, char *);
    dvm_runtime_channel_wake(argument, "duplicate channel wake");
    return 1;
}


/* A malformed receive queue must not partially close the send side. A wake
 * error after detachment must still let the receive side reach the scheduler. */
static void channel_finalize_case(DVM_GCWorkers *workers)
{
    Prog *prior = dvm_runqueue_head();
    DVM_RuntimeProcess foreign = {0}, receiver = {0};
    Channel *channel = H;
    Array *buffer;
    int retained_refs;
    int refs;
    collect(workers, DVM_GC_FAST);
    refs = dvm_refcount_load_i(&Tword.ref);
    check(prior && !prior->link && dvm_runqueue_pop(Psend) == prior,
        "withdraw startup process for channel finalization");
    channel = dvm_runtime_channel_new(&channel, &Tword, 2, movtmp, 0);
    buffer = channel->buf;
    retained_refs = dvm_refcount_load_i(&Tword.ref);
    check(buffer != H && retained_refs > refs, "buffer and descriptor are live before close");
    dvm_runqueue_set_state(&foreign.process, Prelease);
    dvm_runqueue_set_state(&receiver.process, Precv);
    cqadd(&channel->send, &foreign.process);
    cqadd(&channel->recv, &receiver.process);
    channel->recv->next = channel->recv;
    check(!kprotect(finalize_channel, channel) && !strcmp(getup()->env->errstr, exHeap) &&
        channel->send->prog == &foreign.process && channel->recv->prog == &receiver.process &&
        channel->buf == buffer && channel->mover == movtmp &&
        dvm_refcount_load_i(&Tword.ref) == retained_refs &&
        !dvm_runqueue_head(), "finalizer refusal preserves both owners");
    channel->recv->next = NULL;
    receiver.channel_claimed = &receiver;
    check(!kprotect(finalize_channel, channel) &&
        !strcmp(getup()->env->errstr, "channel waiter cannot be claimed") &&
        channel->send->prog == &foreign.process && channel->recv->prog == &receiver.process &&
        receiver.channel_claimed == &receiver && channel->buf == buffer &&
        channel->mover == movtmp && !dvm_runqueue_head(),
        "finalizer refuses a competing claim before detaching either side");
    receiver.channel_claimed = NULL;
    check(!kprotect(finalize_channel, channel) &&
        strstr(getup()->env->errstr, "foreign C channel waiter") &&
        !channel->send && !channel->recv && !channel->mover && channel->buf == H &&
        dvm_refcount_load_i(&Tword.ref) < retained_refs &&
        dvm_runqueue_head() == &receiver.process &&
        !receiver.channel_claimed && !receiver.process.ptr && !receiver.process.chan,
        "finalizer drains both queues and releases descriptor on wake error");
    receiver.channel_claimed = &receiver;
    check(!dvm_runqueue_claim(&receiver.process) && !dvm_runqueue_pop(Psend) &&
        dvm_runqueue_head() == &receiver.process && receiver.process.state == Pready,
        "live channel claim blocks dispatch and queue removal");
    receiver.channel_claimed = NULL;
    check(!kprotect(repeat_channel_wake, &receiver.process) &&
        !strcmp(getup()->env->errstr, "invalid channel waiter state") &&
        !strcmp(receiver.process.kill, "channel hangup") &&
        dvm_runqueue_pop(Psend) == &receiver.process && !dvm_runqueue_head(),
        "duplicate completion preserves the first wake and FIFO");
    check(dvm_runqueue_append(prior), "restore startup process");
    destroy(channel);
    collect(workers, DVM_GC_FAST);
    check(dvm_refcount_load_i(&Tword.ref) == refs, "finalized buffer releases its element descriptor");
    puts("PASS two-sided channel finalization");
}


