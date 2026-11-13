/* Copyright (c) 2026 bhgv
 * EN: ALT publication failure and native process lifetime probes.
 * RU: Проверки ошибок публикации ALT и жизни нативного процесса.
 * Included once by test_record_new.c in declaration order.
 */

/* Invoke the hosted external ALT cancellation in a VM error checkpoint. */
static int cancel_external_alt(void *argument)
{
    altgone(argument);
    return 1;
}


static void *refused_alt_result; /* Borrowed fixture cell selected for BUSY. */
DVM_CellResult __real_dvm_managed_ranges_begin(const void *, size_t, void *, size_t, DVM_RangeAccess *);


/* Refuse the selection claim without modifying either output. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    if (refused_alt_result && destination == refused_alt_result && destination_size == sizeof(WORD))
    {
        refused_alt_result = NULL;
        return DVM_CELL_BUSY;
    }
    return __real_dvm_managed_ranges_begin(source, source_size, destination, destination_size, access);
}


/* Refuse an ALT result only after SEND has removed both registrations. */
static void alt_publication_failure(void)
{
    Prog *prior = dvm_runqueue_head();
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    DVM_RuntimeProcess waiter = {0};
    Alt *guards = calloc(1, offsetof(Alt, ac) + 2*sizeof(Altc));
    Channel *selected = H;
    Channel *sibling = H;
    WORD payload = 47;
    WORD untouched = 61;
    int receive = 0;
    check(guards && prior && !prior->link && dvm_runqueue_pop(Psend) == prior,
        "withdraw startup process for ALT publication probe");
    selected = dvm_runtime_channel_new(&selected, NULL, 0, failing_channel_move, 0);
    sibling = dvm_runtime_channel_new(&sibling, NULL, 0, failing_channel_move, 0);
    guards->nrecv = 2;
    guards->ac[0].c = selected;
    guards->ac[0].ptr = &untouched;
    guards->ac[1].c = sibling;
    guards->ac[1].ptr = &untouched;
    dvm_runqueue_set_state(&waiter.process, Palt);
    waiter.process.R.s = guards;
    waiter.process.R.d = H;
    cqadd(&selected->recv, &waiter.process);
    cqadd(&sibling->recv, &waiter.process);
    registers->s = &payload;
    registers->d = &selected;
    waiter.channel_claimed = &waiter;
    check(!kprotect(direct_channel_transfer, &receive) &&
        !strcmp(getup()->env->errstr, "channel waiter cannot be claimed") &&
        selected->recv->prog == &waiter.process && sibling->recv->prog == &waiter.process &&
        waiter.lifetime_holds == 2 && waiter.channel_claimed == &waiter,
        "competing match leaves both queue holds intact");
    waiter.channel_claimed = NULL;
    sibling->recv->next = sibling->recv;
    check(!kprotect(direct_channel_transfer, &receive) &&
        !strcmp(getup()->env->errstr, exHeap), "reject cyclic sibling queue");
    check(selected->recv->prog == &waiter.process &&
        sibling->recv->prog == &waiter.process && sibling->recv->next == sibling->recv &&
        waiter.lifetime_holds == 2 && !waiter.channel_claimed && waiter.process.state == Palt &&
        !waiter.process.kill && !dvm_runqueue_head() &&
        untouched == 61, "refused ALT cancellation preserves all registrations");
    sibling->recv->next = NULL;
    check(!kprotect(direct_channel_transfer, &receive) &&
        !strcmp(getup()->env->errstr, exBounds), "initiator observes ALT bounds error");
    check(!waiter.lifetime_holds && !selected->recv->prog && !sibling->recv->prog && untouched == 61 &&
        !waiter.process.ptr && waiter.process.kill &&
        !strcmp(waiter.process.kill, "channel transfer failed") &&
        dvm_runqueue_pop(Psend) == &waiter.process && !dvm_runqueue_head(),
        "ALT failure removes both registrations and wakes once");
    {
        WORD result = 71;
        dvm_runqueue_set_state(&waiter.process, Palt);
        waiter.process.kill = NULL;
        waiter.process.R.d = &result;
        cqadd(&selected->recv, &waiter.process);
        cqadd(&sibling->recv, &waiter.process);
        refused_alt_result = &result;
        check(!kprotect(direct_channel_transfer, &receive) &&
            !strcmp(getup()->env->errstr, exHeap) && !refused_alt_result &&
            result == 71 && untouched == 61 && !waiter.channel_claimed &&
            !waiter.lifetime_holds && !selected->recv->prog && !sibling->recv->prog &&
            waiter.process.kill && !strcmp(waiter.process.kill, "channel transfer failed") &&
            dvm_runqueue_pop(Psend) == &waiter.process && !dvm_runqueue_head(),
            "BUSY ALT result preserves output and completes the detached waiter once");
        waiter.process.R.d = H;
    }
    dvm_runqueue_set_state(&waiter.process, Palt);
    waiter.process.kill = NULL;
    cqadd(&selected->recv, &waiter.process);
    cqadd(&sibling->recv, &waiter.process);
    sibling->recv->next = sibling->recv;
    check(!kprotect(cancel_external_alt, &waiter.process) &&
        !strcmp(getup()->env->errstr, exHeap) && !waiter.channel_claimed &&
        waiter.lifetime_holds == 2 && selected->recv->prog == &waiter.process &&
        sibling->recv->prog == &waiter.process && !dvm_runqueue_head(),
        "external ALT cancellation refuses before removing either registration");
    sibling->recv->next = NULL;
    check(kprotect(cancel_external_alt, &waiter.process) &&
        kprotect(cancel_external_alt, &waiter.process) &&
        !waiter.channel_claimed && !waiter.lifetime_holds &&
        !selected->recv->prog && !sibling->recv->prog &&
        waiter.process.kill && !strcmp(waiter.process.kill, "alt channel hungup") &&
        dvm_runqueue_pop(Psend) == &waiter.process && !dvm_runqueue_head(),
        "external ALT cancellation releases both holds and wakes once");
    *registers = saved;
    check(dvm_runqueue_append(prior), "restore startup process");
    free(guards);
    destroy(sibling);
    destroy(selected);
    puts("PASS detached ALT publication failure");
}


typedef struct ProgHoldRace
{
    DVM_RuntimeProcess process; /* Storage kept live by the fixture through join. */
    pthread_barrier_t start;
} ProgHoldRace;


/* Workers touch the native hold word only. They never enter the managed heap. */
static void *race_prog_hold(void *argument)
{
    ProgHoldRace *race = argument;
    unsigned int iteration;
    pthread_barrier_wait(&race->start);
    for (iteration = 0; iteration < 10000; iteration++)
    {
        if (!dvm_runqueue_pin(&race->process.process))
        {
            break;
        }
        dvm_runqueue_unpin(&race->process.process);
    }
    return NULL;
}


/* A live hold rejects retirement. Once admission closes, it cannot reopen. */
static void prog_hold_race(void)
{
    ProgHoldRace race = {0};
    Prog *process = &race.process.process;
    pthread_t workers[4];
    unsigned int index;
    int sealed;
    check(dvm_runqueue_pin(process) && !dvm_runqueue_begin_retire(process),
        "live Prog hold rejects retirement");
    check(!pthread_barrier_init(&race.start, NULL, 5), "initialize Prog hold barrier");
    for (index = 0; index < 4; index++)
    {
        check(!pthread_create(&workers[index], NULL, race_prog_hold, &race), "start Prog hold worker");
    }
    pthread_barrier_wait(&race.start);
    dvm_runqueue_unpin(process);
    sealed = dvm_runqueue_begin_retire(process);
    for (index = 0; index < 4; index++)
    {
        check(!pthread_join(workers[index], NULL), "join Prog hold worker");
    }
    check((sealed || dvm_runqueue_begin_retire(process)) &&
        !dvm_runqueue_pin(process) && !dvm_runqueue_begin_retire(process) &&
        dvm_runqueue_hold_count(process) == -1, "closed Prog hold admission stays closed");
    pthread_barrier_destroy(&race.start);
    puts("PASS native Prog hold race");
}


