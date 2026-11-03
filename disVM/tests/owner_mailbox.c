/* Exclusive ownership moves across OS contexts, without extra retain/drop. */
#include "disvm/owner_slot.h"
#include "disvm/refcount.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>

#define WORKERS 8 /* Real OS contenders for one transferable native owner. */
#define HOPS 500 /* Successful exclusive modifications per worker. */


/* Mutable payload is accessed only through the sole owning local entry. */
typedef struct Token
{
    unsigned long references; /* Must stay exactly one throughout every move. */
    unsigned long hops; /* Sole owner increments; parent observes after joins/take. */
} Token;


/* Parent-owned stable mailbox and start barrier, destroyed after all joins. */
typedef struct Work
{
    DVM_OwnerSlot *slot; /* Sole owner alternates between slot and local entries. */
    FVM_PortSync *gate; /* Protect ready/start, never held across slot operations. */
    int ready; /* Started workers, protected by gate. */
    int start; /* Permit all contenders to begin, protected by gate. */
} Work;

static int retired; /* Written by parent's final drop after every worker joined. */


/* Report an evaluated semantic failure; signal/timeouts are not accepted proof.
 * condition/message borrowed; stop before using any inconsistent owner. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL owner mailbox predicate: %s\n", message);
        exit(10);
    }
}


/* Legal callback for generic slots; a mailbox move must never call it.
 * object is protected by source; retain would make count two and fail oracle. */
static int retain(void *object)
{
    Token *token = object;
    return dvm_refcount_retain_u(&token->references) == DVM_REFCOUNT_LIVE;
}


/* Only parent may drop after all hops and joins. object carries final owner.
 * Fail if any transfer retained/leaked an alias; never inspect after final RMW. */
static void drop(void *object)
{
    Token *token = object;
    check(dvm_refcount_release_u(&token->references) == DVM_REFCOUNT_LAST, "sole final owner");
    free(token);
    retired++;
}

static const DVM_OwnerOps operations = {retain, drop}; /* Stable callbacks, no VM/slot reentry. */


/* Move token into private entry, modify exclusively, publish existing owner.
 * argument points to parent Work alive through join. Nil take yields briefly;
 * the API itself does not wait for a message. No owner exists in an untracked
 * envelope during contention; scopes clean a received token on native unwind. */
static void bounce(void *argument)
{
    Work *work = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    int hop = 0;
    check(context && dvm_native_context_bind(context), "worker context");
    check(dvm_owner_scope_open(&scope), "worker scope");
    fvm_port_sync_lock(work->gate);
    work->ready++;
    fvm_port_sync_signal_all(work->gate);
    while (!work->start)
    {
        fvm_port_sync_wait(work->gate);
    }
    fvm_port_sync_unlock(work->gate);
    while (hop < HOPS)
    {
        Token *token;
        if (!dvm_owner_slot_take(work->slot, &scope, &entry))
        {
            fvm_port_thread_sleep_us(10);
            continue;
        }
        token = entry.object;
        check(token && dvm_refcount_load_u(&token->references) == 1, "exclusive received owner");
        token->hops++;
        check(dvm_owner_slot_put(work->slot, &entry) && !entry.object, "republish sole owner");
        hop++;
    }
    check(dvm_owner_scope_close(&scope), "worker scope empty");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "worker retires");
}


/* Parent retains mailbox lifetime, joins every contender and consumes exactly
 * one final owner. Return0 only after 4000 exclusive updates and real free. */
int main(void)
{
    Work work = {0};
    FVM_PortThread *threads[WORKERS];
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    DVM_OwnerTransfer transfer = {0};
    Token *token = calloc(1, sizeof(*token));
    int index;
    check(token && context && dvm_native_context_bind(context), "parent context/token");
    token->references = 1;
    transfer.object = token;
    transfer.ops = &operations;
    token = NULL;
    work.slot = dvm_owner_slot_create();
    work.gate = fvm_port_sync_create();
    check(work.slot && work.gate && dvm_owner_scope_open(&scope), "parent scope/slot/gate");
    check(dvm_owner_scope_accept(&scope, &entry, &transfer) &&
        dvm_owner_slot_put(work.slot, &entry) && !entry.object, "initial owner publication");
    for (index = 0; index < WORKERS; index++)
    {
        threads[index] = fvm_port_thread_start(bounce, &work);
        check(threads[index] != NULL, "start worker");
    }
    fvm_port_sync_lock(work.gate);
    while (work.ready != WORKERS)
    {
        fvm_port_sync_wait(work.gate);
    }
    work.start = 1;
    fvm_port_sync_signal_all(work.gate);
    fvm_port_sync_unlock(work.gate);
    for (index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_owner_slot_take(work.slot, &scope, &entry), "parent final take");
    token = entry.object;
    check(dvm_refcount_load_u(&token->references) == 1 && token->hops == WORKERS * HOPS,
        "all exclusive updates with no count churn");
    token = NULL;
    check(dvm_owner_scope_close(&scope) && retired == 1, "exactly one final free");
    check(dvm_owner_slot_destroy(&work.slot), "empty joined slot retires");
    fvm_port_sync_destroy(work.gate);
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "parent retires");
    puts("PASS owner mailbox: 8 OS workers, 4000 exclusive updates, count stays one, joined final free");
    return 0;
}
