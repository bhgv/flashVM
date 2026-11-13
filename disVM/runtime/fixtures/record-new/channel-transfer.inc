/* Copyright (c) 2026 bhgv
 * EN: Direct transfer and buffered refill failure probes.
 * RU: Проверки прямой передачи и ошибок пополнения буфера.
 * Included once by test_record_new.c in declaration order.
 */

static DVM_RuntimeProcess *claimed_waiter; /* Live fixture waiter during mover callback. */
static int waiter_notifications; /* Notifications while the direct transfer fixture runs. */
void __real_dvm_managed_workers_wake_guest(Prog *guest);


/* At worker notification, the completed waiter must already be dispatchable. */
void __wrap_dvm_managed_workers_wake_guest(Prog *guest)
{
    if (claimed_waiter)
    {
        check(!claimed_waiter->channel_claimed && !claimed_waiter->lifetime_holds &&
            claimed_waiter->process.state == Pready && !claimed_waiter->process.ptr &&
            !claimed_waiter->process.chan && dvm_runqueue_head() == &claimed_waiter->process,
            "notify only after channel claim and completion pin are released");
        waiter_notifications++;
    }
    __real_dvm_managed_workers_wake_guest(guest);
}


static int unexpected_nested_match(void *argument)
{
    (void)argument;
    check(0, "nested match must not enter");
    return 0;
}


/* Attempt a competing match from inside the current matched transfer. */
static int nested_channel_match(void *argument)
{
    extern void dvm_runtime_channel_with_claim(Prog *, int (*)(void *), void *);
    dvm_runtime_channel_with_claim(argument, unexpected_nested_match, NULL);
    return 1;
}


/* Inject a mover error only after the real opcode dequeues its peer. */
static void failing_channel_move(void)
{
    if (claimed_waiter)
    {
        check(claimed_waiter->channel_claimed && claimed_waiter->lifetime_holds == 1 &&
            !kprotect(nested_channel_match, &claimed_waiter->process) &&
            !strcmp(getup()->env->errstr, "channel waiter cannot be claimed"),
            "live channel claim refuses competing completion");
    }
    error("fixture channel mover failure");
}


/* Call the production C opcode with the active register view. */
static int direct_channel_transfer(void *argument)
{
    int receive = *(int *)argument;
    extern void isend(void), irecv(void);
    if (receive)
    {
        irecv();
    }
    else
    {
        isend();
    }
    return 1;
}


extern void movw(void);
extern void dvm_runtime_channel_queue_guard(int (*)(void *), void *);


/* EN: Check the borrowed channel under its queue mutex after error cleanup.
 * A foreign native owner must see no remaining match. Return one, fatal on error.
 * RU: Проверить заимствованный канал под мьютексом очереди после очистки ошибки.
 * Чужой нативный владелец не должен видеть match. Вернуть один, ошибка фатальна. */
static int channel_match_released(void *channel)
{
    check(!dvm_channel_match_blocked(channel, NULL),
        "foreign owner sees channel match released after transfer cleanup");
    return 1;
}


/* Both direct directions must make a dequeued peer runnable on mover error. */
static void direct_channel_failure(void)
{
    Prog *prior = dvm_runqueue_head();
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    Channel *channel = H;
    WORD payload = 13;
    int receive;
    check(prior && !prior->link && dvm_runqueue_pop(Psend) == prior,
        "withdraw startup process for direct transfer probe");
    channel = dvm_runtime_channel_new(&channel, NULL, 0, failing_channel_move, 0);
    for (receive = 0; receive <= 1; receive++)
    {
        DVM_RuntimeProcess waiter = {0};
        Progq **queue = receive ? &channel->send : &channel->recv;
        dvm_runqueue_set_state(&waiter.process, receive ? Psend : Precv);
        waiter.process.ptr = &payload;
        waiter.process.chan = channel;
        cqadd(queue, &waiter.process);
        registers->s = receive ? (void *)&channel : (void *)&payload;
        registers->d = receive ? (void *)&payload : (void *)&channel;
        waiter_notifications = 0;
        claimed_waiter = &waiter;
        check(!kprotect(direct_channel_transfer, &receive) &&
            strstr(getup()->env->errstr, "fixture channel mover failure"),
            "initiator receives original mover error");
        claimed_waiter = NULL;
        dvm_runtime_channel_queue_guard(channel_match_released, channel);
        check(waiter_notifications == 1 && !waiter.channel_claimed && !waiter.lifetime_holds && !(*queue)->prog && !waiter.process.ptr && !waiter.process.chan &&
            waiter.process.kill && !strcmp(waiter.process.kill, "channel transfer failed") &&
            dvm_runqueue_pop(Psend) == &waiter.process && !dvm_runqueue_head(),
            "failed direct mover wakes the detached peer once");
    }
    *registers = saved;
    check(dvm_runqueue_append(prior), "restore startup process");
    destroy(channel);
    puts("PASS direct channel mover failure");
}


static int refill_moves; /* Counts actual read and refill mover calls. */
static Channel *refill_observed_channel; /* Borrowed during the probe. */
static int refill_fail_at; /* Zero for success, one for read, two for refill. */
static ulong refill_failure_refs; /* Select the intended owner release. */
static void *refill_release_failure; /* Inject only after the temporary channel retain is consumed. */

void __real_destroy(void *value);


/* Report a cleanup error after releasing the selected channel exactly once. */
void __wrap_destroy(void *value)
{
    if (value && value == refill_release_failure &&
        dvm_refcount_load_u(&D2H(value)->ref) == refill_failure_refs)
    {
        refill_release_failure = NULL;
        __real_destroy(value);
        error("fixture channel release failure");
    }
    __real_destroy(value);
}


/* The mover can reenter the queue mutex because reservation has finished. */
static int confirm_unlocked_reservation(void *unused)
{
    (void)unused;
    return 1;
}


/* Move a full native word or fail at the chosen stage before copying it. */
static void staged_refill_move(void)
{
    REG *registers = dvm_native_registers();
    extern void dvm_runtime_channel_queue_guard(int (*)(void *), void *);
    dvm_runtime_channel_queue_guard(confirm_unlocked_reservation, NULL);
    {
        REG saved = *registers;
        WORD nested_output = 0;
        int receive = 1;
        registers->s = &refill_observed_channel;
        registers->d = &nested_output;
        check(!kprotect(direct_channel_transfer, &receive) &&
            strstr(getup()->env->errstr, "channel buffer transfer already active"),
            "nested transfer cannot reuse a reserved slot");
        check(!kprotect(finalize_channel, refill_observed_channel) &&
            strstr(getup()->env->errstr, "channel buffer transfer already active") &&
            refill_observed_channel->send && refill_observed_channel->recv,
            "finalization cannot detach a reserved ring");
        *registers = saved;
    }
    refill_moves++;
    check(refill_observed_channel->size == (refill_moves == 1 ? 1 : 0),
        "ring occupancy is unchanged until the current mover completes");
    if (refill_moves == refill_fail_at)
    {
        error("fixture buffered transfer failure");
    }
    memmove(registers->d, registers->s, sizeof(WORD));
}


/* Exercise real RECV with a full ring and a waiting sender. */
static void buffered_channel_completion(void)
{
    Prog *prior = dvm_runqueue_head();
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    Channel *channel = H;
    int receive = 1;
    int stage;
    check(prior && !prior->link && dvm_runqueue_pop(Psend) == prior,
        "withdraw startup process for buffered transfer probe");
    for (stage = 0; stage <= 4; stage++)
    {
        DVM_RuntimeProcess waiter = {0};
        WORD buffered = 17;
        WORD pending = 31;
        WORD received = -1;
        WORD slot;
        int ok;
        channel = dvm_runtime_channel_new(&channel, &Tword, 1, staged_refill_move, 0);
        memmove(channel->buf->data, &buffered, sizeof(buffered));
        channel->size = 1;
        dvm_runqueue_set_state(&waiter.process, Psend);
        waiter.process.ptr = &pending;
        waiter.process.chan = channel;
        cqadd(&channel->send, &waiter.process);
        refill_observed_channel = channel;
        refill_moves = 0;
        refill_fail_at = stage == 4 ? 2 : stage == 3 ? 0 : stage;
        refill_failure_refs = 2;
        refill_release_failure = stage >= 3 ? channel : NULL;
        registers->s = &channel;
        registers->d = &received;
        ok = kprotect(direct_channel_transfer, &receive);
        dvm_runtime_channel_queue_guard(channel_match_released, channel);
        check(ok == (stage == 0) && refill_moves == (stage == 1 ? 1 : 2),
            "buffered transfer reaches selected stage");
        check(!waiter.channel_claimed && dvm_refcount_load_u(&D2H(channel)->ref) == 1 && !channel->send->prog &&
            !waiter.process.ptr && dvm_runqueue_pop(Psend) == &waiter.process &&
            !dvm_runqueue_head(), "buffered transfer consumes its hold and wakes once");
        check(!refill_release_failure, "selected release failure was exercised");
        check(received == (stage == 1 ? -1 : buffered), "receive retains completed read");
        if (refill_fail_at)
        {
            check(channel->size == (stage == 1 ? 1 : 0),
                "failed read retains its slot and failed refill publishes none");
            if (stage == 1)
            {
                memmove(&slot, channel->buf->data, sizeof(slot));
                check(slot == buffered, "failed read preserves the buffered word");
            }
            check(!waiter.process.chan && waiter.process.kill &&
                !strcmp(waiter.process.kill, "channel transfer failed") &&
                strstr(getup()->env->errstr, "fixture buffered transfer failure"),
                "buffered failure reaches sender and receiver");
        }
        else
        {
            memmove(&slot, channel->buf->data, sizeof(slot));
            check(!waiter.process.kill && !waiter.process.chan && channel->size == 1 && slot == pending,
                "successful refill publishes pending payload");
            if (stage == 3)
            {
                check(strstr(getup()->env->errstr, "fixture channel release failure") != NULL,
                    "receiver observes release failure after successful refill");
            }
        }
        if (stage == 1)
        {
            refill_moves = 0;
            refill_fail_at = 0;
            registers->s = &channel;
            registers->d = &received;
            check(kprotect(direct_channel_transfer, &receive) && refill_moves == 1 &&
                received == buffered && channel->size == 0,
                "retry receives the preserved buffered word");
        }
        /* EN: Reuse this exact channel after every success/error stage.
         * RU: Повторно использовать тот же канал после каждого исхода. */
        channel->mover = movw;
        registers->s = &channel;
        registers->d = &received;
        if (channel->size)
        {
            check(kprotect(direct_channel_transfer, &receive) && !channel->size,
                "recovered channel drains the preserved or committed slot");
        }
        {
            WORD fresh = 43;
            int send = 0;
            received = -1;
            registers->s = &fresh;
            registers->d = &channel;
            check(kprotect(direct_channel_transfer, &send) && channel->size == 1,
                "recovered channel admits a fresh send");
            registers->s = &channel;
            registers->d = &received;
            check(kprotect(direct_channel_transfer, &receive) && received == fresh && !channel->size,
                "recovered channel receives fresh payload exactly once");
        }
    }
    puts("PASS buffered channel recovery on same channel");
    *registers = saved;
    check(dvm_runqueue_append(prior), "restore startup process");
    destroy(channel);
    refill_observed_channel = NULL;
    puts("PASS buffered channel completion");
}


