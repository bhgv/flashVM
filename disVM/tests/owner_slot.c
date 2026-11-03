/* Real malloc lifetimes under simultaneous native slot copying/replacement. */
#include "disvm/owner_slot.h"
#include "disvm/refcount.h"
#include "disvm/object_guard.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define READERS 8 /* Real native threads, each with its own bound context. */
#define COPIES 2000 /* Reads per thread while writer replaces the shared owner. */
#define REPLACEMENTS 1000 /* Independently allocated messages, no permanent payload arena. */


/* Immutable after publication; its count protects allocation lifetime. */
typedef struct Message
{
    unsigned long references; /* One count per slot/entry/transfer owner. */
    unsigned long value; /* Immutable diagnostic, no mutable-payload permission. */
} Message;


/* Parent storage outlives complete joins; gate protects the start barrier only. */
typedef struct Work
{
    DVM_OwnerSlot *slot; /* Shared stable address; ownership fields stay opaque. */
    FVM_PortSync *gate; /* Ready/start barrier, never held during slot operations. */
    int ready; /* Readers waiting, protected by gate. */
    int start; /* Release readers, protected by gate. */
} Work;

static unsigned long retired = 1; /* Atomic diagnostic with nonzero refcount baseline. */


/* Exact semantic predicate; compiler errors, signals/timeouts are not controls.
 * condition/message borrowed; terminate before invalid owner state can be used. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL owner slot predicate: %s\n", message);
        exit(10);
    }
}


/* Retain an already protected source message. Return unchanged on overflow/zero.
 * Callback is bounded native work, with no slot/VM/admission reentry. */
static int retain(void *object)
{
    Message *message = object;
    return dvm_refcount_retain_u(&message->references) == DVM_REFCOUNT_LIVE;
}


/* Consume an owner outside the slot mutex; final release frees real storage.
 * No message access follows either successful count RMW. */
static void drop(void *object)
{
    Message *message = object;
    DVM_RefcountResult result = dvm_refcount_release_u(&message->references);
    check(result != DVM_REFCOUNT_REFUSED, "drop valid owner");
    if (result == DVM_REFCOUNT_LAST)
    {
        free(message);
        check(dvm_refcount_retain_u(&retired) == DVM_REFCOUNT_LIVE, "record retirement");
    }
}

static const DVM_OwnerOps operations = {retain, drop}; /* Immutable callbacks, process lifetime. */


/* Allocate one private immutable message, returning its sole existing owner.
 * value copied; failure terminates fixture. Caller moves/cancels returned owner. */
static DVM_OwnerTransfer create(unsigned long value)
{
    DVM_OwnerTransfer transfer;
    Message *message = malloc(sizeof(*message));
    check(message != NULL, "allocate message");
    message->references = 1;
    message->value = value;
    transfer.object = message;
    transfer.ops = &operations;
    return transfer;
}


/* Each reader has independent scope/entry. Copy completes before payload read;
 * no count is acquired through an unprotected raw slot pointer. */
static void read_slot(void *argument)
{
    Work *work = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    int iteration;
    check(context && dvm_native_context_bind(context), "reader context");
    check(dvm_owner_scope_open(&scope), "reader scope");
    fvm_port_sync_lock(work->gate);
    work->ready++;
    fvm_port_sync_signal_all(work->gate);
    while (!work->start)
    {
        fvm_port_sync_wait(work->gate);
    }
    fvm_port_sync_unlock(work->gate);
    for (iteration = 0; iteration < COPIES; iteration++)
    {
        Message *message;
        check(dvm_owner_slot_copy(work->slot, &scope, &entry), "reader retained snapshot");
        message = entry.object;
        check(message->value <= REPLACEMENTS, "immutable retained payload");
        check(dvm_owner_scope_drop(&entry), "reader drops snapshot");
    }
    check(dvm_owner_scope_close(&scope), "reader scope close");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "reader context retires");
}


/* Exercise deterministic ownership predicates before concurrent real lifetimes.
 * No arguments. Return 0 only after every thread joins and allocation retires. */
int main(void)
{
    Work work = {0};
    FVM_PortThread *threads[READERS];
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    DVM_OwnerTransfer transfer = {0};
    DVM_OwnerTransfer malformed = {0};
    Message *first;
    DVM_ObjectDomain *domain;
    DVM_ObjectGuard guard = {0};
    int index;
    check(context && dvm_native_context_bind(context), "writer context");
    check(dvm_owner_scope_open(&scope), "writer scope");
    work.slot = dvm_owner_slot_create();
    work.gate = fvm_port_sync_create();
    check(work.slot && work.gate, "slot and gate allocation");
    check(!dvm_owner_slot_copy(work.slot, &scope, &entry) && !entry.object, "nil copy unchanged");
    transfer = create(0);
    first = transfer.object;
    check(dvm_owner_slot_exchange(work.slot, &transfer), "publish first owner");
    check(!transfer.object && !transfer.ops, "first exchange consumes incoming owner");
    check(!dvm_owner_slot_destroy(&work.slot), "populated destroy refuses");
    check(dvm_owner_slot_copy(work.slot, &scope, &entry), "copy first owner");
    check(dvm_refcount_load_u(&first->references) == 2, "copy creates DISTINCT owner");
    check(!dvm_owner_slot_copy(work.slot, &scope, &entry), "occupied destination refuses");
    check(dvm_owner_scope_drop(&entry), "drop first snapshot");
    /* Quiescent single caller still has the stable slot owner. Set only for
     * this refusal test; no concurrent readers exist until after restoration. */
    first->references = ULONG_MAX;
    check(!dvm_owner_slot_copy(work.slot, &scope, &entry) && !entry.object &&
        dvm_refcount_load_u(&first->references) == ULONG_MAX, "overflow leaves both owners unchanged");
    first->references = 1;
    first = NULL;
    domain = dvm_object_domain_create();
    check(domain && dvm_object_guard_init(&guard, domain), "guard setup");
    check(dvm_object_guard_try_enter(&guard) == DVM_OBJECT_ACQUIRED, "hold guard");
    check(!dvm_owner_slot_copy(work.slot, &scope, &entry) &&
        !dvm_owner_slot_exchange(work.slot, &transfer), "guarded copy and exchange refuse");
    check(dvm_object_guard_leave(&guard) && dvm_object_guard_fini(&guard) &&
        dvm_object_domain_destroy(domain), "guard teardown");
    malformed.ops = &operations;
    check(!dvm_owner_slot_exchange(work.slot, &malformed) && malformed.ops == &operations,
        "malformed input unchanged");
    check(dvm_owner_scope_close(&scope), "close before temporary unbind");
    check(dvm_native_context_unbind(context), "temporary unbind");
    check(!dvm_owner_slot_exchange(work.slot, &transfer), "unbound exchange refuses");
    check(dvm_native_context_bind(context), "writer rebind");
    check(dvm_owner_scope_open(&scope), "writer reopens scope");
    for (index = 0; index < READERS; index++)
    {
        threads[index] = fvm_port_thread_start(read_slot, &work);
        check(threads[index] != NULL, "start reader");
    }
    fvm_port_sync_lock(work.gate);
    while (work.ready != READERS)
    {
        fvm_port_sync_wait(work.gate);
    }
    work.start = 1;
    fvm_port_sync_signal_all(work.gate);
    fvm_port_sync_unlock(work.gate);
    for (index = 1; index <= REPLACEMENTS; index++)
    {
        transfer = create((unsigned long)index);
        check(dvm_owner_slot_exchange(work.slot, &transfer), "replace shared owner");
        check(transfer.object && transfer.ops == &operations, "previous owner returned");
        check(dvm_owner_transfer_cancel(&transfer), "retire replaced owner outside mutex");
    }
    for (index = 0; index < READERS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_owner_slot_exchange(work.slot, &transfer), "assign nil after joins");
    check(transfer.object && dvm_owner_transfer_cancel(&transfer), "retire last slot owner");
    check(dvm_refcount_load_u(&retired) == REPLACEMENTS + 2, "all allocations retired exactly once");
    check(dvm_owner_slot_destroy(&work.slot) && !work.slot, "empty quiescent destruction");
    fvm_port_sync_destroy(work.gate);
    check(dvm_owner_scope_close(&scope), "writer scope close");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "writer context retires");
    puts("PASS owner slot: 8 OS readers, 16000 snapshots, 1001 malloc/free lifetimes, complete joins");
    return 0;
}
