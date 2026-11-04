/* Exercise real record construction, including a writer inside the monitor. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "runtab-legacy.h"
#include "disvm/native_context.h"
#include "raise.h"
#include "record_new.h"
#include "heap_commit.h"
#include "shared_pointer.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "runqueue.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <pthread.h>
#include "pointer_builder.h"
#include <stdlib.h>
#include <string.h>

static Type *record; /* Genuine descriptor retained through the probe. */
static void *destination; /* Owning field in a rooted List. */
static int mode; /* 1 interleaves a writer, 2 raises, 3 drops module, 4 drops module and raises. */
static void *module_slot; /* Rooted source slot for the imported opcode. */
static Modlink *imported; /* Borrowed through the imported constructor hold. */
static int callbacks; /* Complete record allocations observed by monitor. */
static int stale; /* Actual failed identity comparisons. */
static int array_mode; /* Inspect arrays instead of records in the monitor. */
static Type *reject_root_type; /* Fail the candidate handoff after initialization. */
static Heap *previous_owner; /* Existing owner excluded from candidate refusal. */
static int root_refusals; /* Counts candidate-specific refusal injections. */
static WORD length; /* Requested array extent in the current diagnostic. */
static int guest_entries; /* Real adapted opcode calls after the probe. */

extern void mnewz(void); /* Real hosted opcode with decoded operands. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
void __real_dvm_runtime_record_new(void *,Type *,int);
int __real_dvm_runtime_heap_commit(void *,const DVM_HeapHold *,DVM_HeapHold *,DVM_HeapHold *);


/* Stop before dereferencing an invalid candidate or allowing collection. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL record construction: %s\n",message);
        exit(10);
    }
}


/* Count real opcode dispatch outside the native diagnostic. */
void __wrap_dvm_runtime_record_new(void *slot, Type *type, int clear)
{
    if (!record && ++guest_entries == 1)
    {
        REG *registers = dvm_native_registers();
        Jit *jit = registers->M->m->jit;
        if (jit)
        {
            check(!jit->entry[INEW] && !jit->entry[INEWZ] && !jit->entry[IMNEWZ] &&
                !jit->entry[INEWA] && !jit->entry[INEWAZ] &&
                !jit->entry[INEWCB] && !jit->entry[INEWCW] && !jit->entry[INEWCL] &&
                !jit->entry[INEWCF] && !jit->entry[INEWCP] &&
                !jit->entry[INEWCM] && !jit->entry[INEWCMP] &&
                !jit->entry[ISEND] && !jit->entry[IRECV],
                "guarded allocation and transfer JIT use checked dispatch");
            puts("PASS guarded record JIT cache");
        }
        puts("PASS record opcode route");
    }
    __real_dvm_runtime_record_new(slot,type,clear);
}


/* Observe the real array opcode before calling the shared allocator. */
void __real_dvm_runtime_array_new(void *, Type *, WORD, int);
void __wrap_dvm_runtime_array_new(void *slot, Type *type, WORD count, int clear)
{
    if (!record)
    {
        puts("PASS array opcode route");
    }
    __real_dvm_runtime_array_new(slot, type, count, clear);
}


/* Refuse only the fully initialized candidate root of the selected type. */
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *, void *const *, size_t);
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count)
{
    if (reject_root_type && count == 1 && keys[0] != previous_owner &&
        ((Heap *)keys[0])->t == reject_root_type)
    {
        root_refusals++;
        return 0;
    }
    return __real_dvm_runtime_root_batch_publish(batch, keys, count);
}


/* Preserve real publication and count its stale result for this field only. */
int __wrap_dvm_runtime_heap_commit(void *slot, const DVM_HeapHold *observed,
    DVM_HeapHold *incoming, DVM_HeapHold *retired)
{
    int result = __real_dvm_runtime_heap_commit(slot,observed,incoming,retired);
    if (slot == destination && result == 0)
    {
        stale++;
    }
    return result;
}


/* Observe H and scalar zero before a separate compliant write or VM error. */
static void monitor(int event, void *value, ulong bytes)
{
    Heap *header = value;
    void *head;
    void *previous;
    void *empty = H;
    WORD scalar;
    if (event || header->t != (array_mode ? &Tarray : record))
    {
        return;
    }
    callbacks++;
    check(dvm_runtime_root_count(header) == 1 &&
        !dvm_native_context_object_count(dvm_native_context_current()), "rooted unguarded monitor");
    if (array_mode)
    {
        Array *array = H2D(Array *, header);
        WORD index;
        check(array->len == length && array->t == record && array->root == H &&
            array->data == (uchar *)array + sizeof(*array) &&
            bytes == sizeof(*array) + (ulong)length * record->size, "complete array header");
        for (index = 0; index < length; index++)
        {
            uchar *element = array->data + index * record->size;
            memmove(&head, element, sizeof(head));
            memmove(&scalar, element + sizeof(head), sizeof(scalar));
            check(head == H && scalar == 0, "initialized array element");
        }
    }
    else
    {
        check(bytes == record->size, "complete record extent");
        memmove(&head, H2D(void *, header), sizeof(head));
        memmove(&scalar, H2D(uchar *, header) + sizeof(head), sizeof(scalar));
        check(head == H && scalar == 0, "initialized mapped and scalar fields");
    }
    if (mode == 1)
    {
        check(!dvm_runtime_movp_copy(&empty,destination,&previous),"interleaved guarded writer");
        destroy(previous);
    }
    if (mode == 3 || mode == 4)
    {
        check(dvm_runtime_root_count(D2H(imported)) == 1, "imported module lease");
        check(!dvm_runtime_movp_copy(&empty,module_slot,&previous) && previous == imported,
            "remove imported module from the application slot");
        destroy(previous);
        check(dvm_runtime_root_count(D2H(imported)) == 1, "module survives source replacement");
    }
    if (mode == 2 || mode == 4)
    {
        error("record monitor primary");
    }
}


/* Invoke the real coordinator under the existing VM error checkpoint. */
static int construct(void *argument)
{
    (void)argument;
    dvm_runtime_record_new(destination,record,1);
    return 1;
}


/* Run actual collection after parking this owner. */
static void collect(DVM_GCWorkers *workers, int kind)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,kind) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"collector progress");
}


/* Invoke the real opcode through the VM checkpoint. */
static int construct_imported(void *argument)
{
    (void)argument;
    mnewz();
    return 1;
}


/* Invoke MNEWZ using the active register set and a real Modlink/Module. */
static void imported_case(DVM_GCWorkers *workers, int fail)
{
    char name[] = "$FlashVM-record-import-test";
    Runtab table = {0};
    Import imports[1] = {{0}};
    Module *module = builtinmod(name, &table, 0);
    List *container = H;
    void *empty = H;
    void *previous;
    WORD index = 0;
    void *before;
    int completed;
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    int baseline = dvm_refcount_load_i(&Tmodlink.ref);
    check(module != NULL, "real imported module");
    module->type = mallocz(sizeof(*module->type), 1);
    check(module->type != NULL &&
        dvm_refcount_retain_i(&record->ref) == DVM_REFCOUNT_LIVE, "module descriptor owner");
    module->type[0] = record;
    module->ntype = 1;
    imported = linkmod(lookmod(name), imports, 1);
    check(imported != H, "real imported Modlink");
    unload(module);
    dvm_runtime_pointer_cons(&empty, &container);
    ptradd(D2H(container));
    module_slot = container->data;
    check(!dvm_runtime_movp_copy(&imported, module_slot, &previous) && previous == H,
        "publish source owner");
    destroy(imported);
    registers->s = module_slot;
    registers->m = &index;
    registers->d = destination;
    memmove(&before, destination, sizeof(before));
    mode = fail ? 4 : 3;
    completed = kprotect(construct_imported, NULL);
    check(completed == !fail, "imported opcode outcome");
    if (fail)
    {
        memmove(&previous, destination, sizeof(previous));
        check(previous == before && strstr(getup()->env->errstr,"record monitor primary"),
            "imported error preserves destination and primary message");
    }
    *registers = saved;
    heapmonitor = NULL;
    memmove(&previous, module_slot, sizeof(previous));
    check(previous == H, "source write retained");
    ptrdel(D2H(container));
    destroy(container);
    collect(workers, DVM_GC_FAST);
    check(lookmod(name) == NULL && dvm_refcount_load_i(&Tmodlink.ref) == baseline,
        "import lease drains through actual unload");
    module_slot = NULL;
    imported = NULL;
    puts("PASS imported record lease");
}


/* Run the checked array allocator within the VM error checkpoint. */
static int construct_array(void *unused)
{
    (void)unused;
    dvm_runtime_array_new(destination, record, length, 1);
    return 1;
}


/* Exercise initialized mapped elements, stale publication, overflow and rollback. */
static void array_case(DVM_GCWorkers *workers)
{
    int before_array = dvm_refcount_load_i(&Tarray.ref);
    int before_element = dvm_refcount_load_i(&record->ref);
    int before_callbacks = callbacks;
    void *published;
    void *current;
    array_mode = 1;
    mode = 1;
    length = 3;
    heapmonitor = monitor;
    check(kprotect(construct_array, NULL) && stale == 2 && callbacks == before_callbacks+1,
        "array reuses candidate after stale comparison");
    collect(workers, DVM_GC_FAST);
    memmove(&published, destination, sizeof(published));
    check(D2H(published)->t == &Tarray && dvm_refcount_load_i(&Tarray.ref) == before_array+1 &&
        dvm_refcount_load_i(&record->ref) == before_element, "array and element descriptors owned");

    mode = 0;
    previous_owner = D2H(published);
    reject_root_type = &Tarray;
    check(!kprotect(construct_array, NULL) && root_refusals == 1, "candidate root refusal");
    reject_root_type = NULL;
    check(dvm_refcount_load_i(&Tarray.ref) == before_array+1 &&
        dvm_refcount_load_i(&record->ref) == before_element, "private descriptors rolled back");
    length = -1;
    check(!kprotect(construct_array, NULL), "negative extent rejected");
    length = 0x7fffffff;
    check(!kprotect(construct_array, NULL), "overflow extent rejected");
    if (sizeof(WORD) > 4)
    {
        length = (WORD)(((uvlong)1 << 32) + 3);
        check(!kprotect(construct_array, NULL), "full native count checked before narrowing");
    }
    memmove(&current, destination, sizeof(current));
    check(current == published && callbacks == before_callbacks+1, "refusal preserves publication");

    mode = 2;
    length = 3;
    check(!kprotect(construct_array, NULL) && strstr(getup()->env->errstr, "record monitor primary"),
        "array monitor error preserved");
    collect(workers, DVM_GC_FAST);
    memmove(&current, destination, sizeof(current));
    check(current == published && dvm_refcount_load_i(&Tarray.ref) == before_array+1 &&
        dvm_refcount_load_i(&record->ref) == before_element, "array monitor failure retires candidate");

    mode = 0;
    length = 0;
    check(kprotect(construct_array, NULL), "empty array publication");
    memmove(&current, destination, sizeof(current));
    check(((Array *)current)->len == 0, "zero count preserved");
    heapmonitor = NULL;
    array_mode = 0;
    previous_owner = NULL;
    puts("PASS guarded arrays");
}


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
    foreign.process.state = Prelease;
    alternative.process.state = Palt;
    sender.process.state = Psend;
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
static void channel_finalize_case(void)
{
    Prog *prior = dvm_runqueue_head();
    DVM_RuntimeProcess foreign = {0}, receiver = {0};
    Channel *channel = H;
    int refs = dvm_refcount_load_i(&Tword.ref);
    check(prior && !prior->link && dvm_runqueue_pop(Psend) == prior,
        "withdraw startup process for channel finalization");
    channel = dvm_runtime_channel_new(&channel, &Tword, 0, movtmp, 0);
    foreign.process.state = Prelease;
    receiver.process.state = Precv;
    cqadd(&channel->send, &foreign.process);
    cqadd(&channel->recv, &receiver.process);
    channel->recv->next = channel->recv;
    check(!kprotect(finalize_channel, channel) && !strcmp(getup()->env->errstr, exHeap) &&
        channel->send->prog == &foreign.process && channel->recv->prog == &receiver.process &&
        channel->mover == movtmp && dvm_refcount_load_i(&Tword.ref) == refs+1 &&
        !dvm_runqueue_head(), "finalizer refusal preserves both owners");
    channel->recv->next = NULL;
    receiver.channel_claimed = &receiver;
    check(!kprotect(finalize_channel, channel) &&
        !strcmp(getup()->env->errstr, "channel waiter cannot be claimed") &&
        channel->send->prog == &foreign.process && channel->recv->prog == &receiver.process &&
        receiver.channel_claimed == &receiver && !dvm_runqueue_head(),
        "finalizer refuses a competing claim before detaching either side");
    receiver.channel_claimed = NULL;
    check(!kprotect(finalize_channel, channel) &&
        strstr(getup()->env->errstr, "foreign C channel waiter") &&
        !channel->send && !channel->recv && !channel->mover &&
        dvm_refcount_load_i(&Tword.ref) == refs &&
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
    puts("PASS two-sided channel finalization");
}


static DVM_RuntimeProcess *claimed_waiter; /* Live fixture waiter during mover callback. */


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
        waiter.process.state = receive ? Psend : Precv;
        waiter.process.ptr = &payload;
        waiter.process.chan = channel;
        cqadd(queue, &waiter.process);
        registers->s = receive ? (void *)&channel : (void *)&payload;
        registers->d = receive ? (void *)&payload : (void *)&channel;
        claimed_waiter = &waiter;
        check(!kprotect(direct_channel_transfer, &receive) &&
            strstr(getup()->env->errstr, "fixture channel mover failure"),
            "initiator receives original mover error");
        claimed_waiter = NULL;
        check(!waiter.channel_claimed && !waiter.lifetime_holds && !(*queue)->prog && !waiter.process.ptr && !waiter.process.chan &&
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
        waiter.process.state = Psend;
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
    }
    *registers = saved;
    check(dvm_runqueue_append(prior), "restore startup process");
    destroy(channel);
    refill_observed_channel = NULL;
    puts("PASS buffered channel completion");
}


/* Invoke the hosted external ALT cancellation in a VM error checkpoint. */
static int cancel_external_alt(void *argument)
{
    altgone(argument);
    return 1;
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
    waiter.process.state = Palt;
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
    waiter.process.state = Palt;
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


/* Execute the real channel mover with the fixture's decoded operands. */
static int mapped_channel_move(void *unused)
{
    (void)unused;
    movtmp();
    return 1;
}


/* The guarded mover refuses an invalid mapped source before changing output. */
static void mapped_channel_move_case(void)
{
    struct {
        void *pointer;
        WORD scalar;
    } source = {NULL, 17}, destination_value = {H, 29};
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    registers->s = &source;
    registers->d = &destination_value;
    registers->m = &record;
    check(!kprotect(mapped_channel_move, NULL) &&
        destination_value.pointer == H && destination_value.scalar == 29,
        "mapped channel mover refusal leaves destination intact");
    source.pointer = H;
    check(kprotect(mapped_channel_move, NULL) &&
        destination_value.pointer == H && destination_value.scalar == 17,
        "mapped channel mover copies validated fields");
    *registers = saved;
    puts("PASS guarded mapped channel mover");
}


/* A mapped send must leave the empty ring unchanged when ownership fails. */
static void mapped_channel_send_case(void)
{
    struct {
        void *pointer;
        WORD scalar;
    } source = {NULL, 73};
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    Channel *channel = H;
    int receive = 0;
    void *pointer;
    WORD scalar;
    channel = dvm_runtime_channel_new(&channel, record, 1, movtmp, 0);
    registers->s = &source;
    registers->d = &channel;
    check(!kprotect(direct_channel_transfer, &receive) && channel->size == 0,
        "refused mapped send does not publish occupancy");
    memmove(&pointer, channel->buf->data, sizeof(pointer));
    check(pointer == H, "refused mapped send preserves empty slot");
    source.pointer = H;
    registers->s = &source;
    registers->d = &channel;
    check(kprotect(direct_channel_transfer, &receive) && channel->size == 1,
        "successful mapped send publishes occupancy");
    memmove(&scalar, channel->buf->data + sizeof(pointer), sizeof(scalar));
    check(scalar == 73, "published mapped slot contains the complete payload");
    *registers = saved;
    destroy(channel);
    puts("PASS mapped channel send publication");
}


/* Check both sides of the ownership commit through the actual SEND/RECV. */
static void mapped_channel_receive_case(void)
{
    struct {
        void *pointer;
        WORD scalar;
    } source = {H, 83}, output = {NULL, 31};
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    Channel *channel = H;
    int receive = 0;
    void *incoming;
    void *slot;
    channel = dvm_runtime_channel_new(&channel, record, 1, movtmp, 0);
    dvm_runtime_record_new(&source.pointer, record, 1);
    incoming = source.pointer;
    registers->s = &source;
    registers->d = &channel;
    check(kprotect(direct_channel_transfer, &receive) && channel->size == 1,
        "seed mapped ring with an owned reference");
    destroy(source.pointer);
    source.pointer = H;
    receive = 1;
    registers->s = &channel;
    registers->d = &output;
    check(!kprotect(direct_channel_transfer, &receive) && channel->size == 1 &&
        !output.pointer && output.scalar == 31, "failed mapped read preserves occupancy");
    memmove(&slot, channel->buf->data, sizeof(slot));
    check(slot == incoming, "failed mapped read retains source ownership");
    output.pointer = H;
    dvm_runtime_record_new(&output.pointer, record, 1);
    refill_failure_refs = 1;
    refill_release_failure = output.pointer;
    registers->s = &channel;
    registers->d = &output;
    check(!kprotect(direct_channel_transfer, &receive) && !refill_release_failure &&
        strstr(getup()->env->errstr, "fixture channel release failure") &&
        channel->size == 0 && output.pointer == incoming && output.scalar == 83,
        "committed mapped read publishes despite displaced-owner error");
    memmove(&slot, channel->buf->data, sizeof(slot));
    check(slot == H, "committed mapped read clears the ring owner");
    *registers = saved;
    destroy(output.pointer);
    destroy(channel);
    puts("PASS mapped channel receive ownership");
}


static Channel **buffer_owner_slot; /* Test application owner. */


/* Drop the application's only channel reference from a real buffered mover. */
static void drop_buffer_owner(void)
{
    Channel *channel = *buffer_owner_slot;
    REG *registers = dvm_native_registers();
    check(dvm_runtime_root_count(D2H(channel)) == 1, "buffer mover has a rooted channel");
    *buffer_owner_slot = H;
    destroy(channel);
    check(dvm_runtime_root_count(D2H(channel)) == 1 && channel->size == 0,
        "buffer reservation survives loss of application owner");
    memmove(registers->d, registers->s, sizeof(WORD));
}


/* The reservation keeps the ring alive until metadata has been published. */
static void buffer_owner_case(void)
{
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    Channel *channel = H;
    WORD value = 53;
    int receive = 0;
    channel = dvm_runtime_channel_new(&channel, &Tword, 1, drop_buffer_owner, 0);
    buffer_owner_slot = &channel;
    registers->s = &value;
    registers->d = &channel;
    check(kprotect(direct_channel_transfer, &receive) && channel == H,
        "send completes after application owner disappears");
    buffer_owner_slot = NULL;
    *registers = saved;
    puts("PASS rooted channel buffer lifetime");
}


/* Use real Type and List storage, then check counts after deferred retirement. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    uchar map = 0x80;
    List *container = H;
    void *empty = H;
    void *published;
    void *current;
    int array_refs;
    int channel_refs;
    __real_dvm_runtime_root_registry_probe(workers);
    array_refs = dvm_refcount_load_i(&Tarray.ref);
    channel_refs = dvm_refcount_load_i(&Tchannel.ref);
    record = dtype(freeheap,sizeof(void *)+sizeof(WORD),&map,1);
    check(record != NULL,"real descriptor");
    dvm_runtime_pointer_cons(&empty,&container);
    ptradd(D2H(container));
    destination = container->data;
    dvm_runtime_record_new(destination,record,0);
    heapmonitor = monitor;
    mode = 1;
    check(kprotect(construct,NULL) && stale == 1 && callbacks == 1,"same candidate after stale");
    memmove(&published,destination,sizeof(published));
    mode = 2;
    check(!kprotect(construct,NULL) && callbacks == 2 &&
        strstr(getup()->env->errstr,"record monitor primary"),"monitor error preserved");
    memmove(&current,destination,sizeof(current));
    check(current == published,"monitor error preserves destination");
    heapmonitor = NULL;
    collect(workers,DVM_GC_CYCLES);
    memmove(&current,destination,sizeof(current));
    check(current == published && D2H(current)->t == record,"published record survives Cycles");
    heapmonitor = monitor;
    imported_case(workers, 0);
    heapmonitor = monitor;
    imported_case(workers, 1);
    array_case(workers);
    channel_case();
    mapped_channel_move_case();
    mapped_channel_send_case();
    mapped_channel_receive_case();
    prog_hold_race();
    channel_drain_case();
    channel_finalize_case();
    direct_channel_failure();
    buffered_channel_completion();
    buffer_owner_case();
    alt_publication_failure();
    ptrdel(D2H(container));
    destroy(container);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&record->ref) == 1 &&
        dvm_refcount_load_i(&Tarray.ref) == array_refs &&
        dvm_refcount_load_i(&Tchannel.ref) == channel_refs, "descriptor baselines restored");
    freetype(record);
    record = NULL;
    destination = NULL;
    puts("PASS rooted record construction");
}
