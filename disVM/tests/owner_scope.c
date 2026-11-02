/* Real native owners, aliases, transfers and final frees; no managed heap model. */
#include "disvm/owner_scope.h"
#include "disvm/object_guard.h"
#include "disvm/refcount.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define WORKERS 8 /* Actual OS threads, each with two owned aliases. */
#define LIFETIMES 100 /* Real last-release/free handoffs per backend. */


/* Shared counters are synchronized separately from payload ownership. */
typedef struct Message
{
    unsigned long references; /* Every linked entry/envelope owns exactly one. */
    int value;                /* Immutable after native thread publication. */
} Message;

static FVM_PortSync *gate; /* Native test barrier protecting ready/go. */
static int ready; /* Number of workers with both aliases, under gate. */
static int go; /* Main permits cleanup, under gate. */
static unsigned long retired; /* Atomic final-free count, never owns an object. */
static DVM_OwnerScope *probe_scope; /* Owner-local callback reentry target or NULL. */
static DVM_OwnerEntry probe_entry; /* Private zeroed slot for rejected reentry. */
static Message *probe_object; /* Borrowed while callback is protected by caller root. */
static int order_expected; /* Owner-local expected LIFO payload value, or zero outside order test. */
static int probe; /* Owner-local retain/drop callback reentry check. */


/* Abort with semantic status 10. condition is evaluated by caller; label borrows
 * static text. Compiler/signal/timeout cannot count as a successful control. */
static void check(int condition, const char *label)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL owner scope predicate: %s\n", label);
        exit(10);
    }
}


/* Retain one already-live message. object borrowed with caller/source protection;
 * return false unchanged for zero/overflow. Never inspect freed payload. */
static int retain(void *object)
{
    Message *message = object;
    return dvm_refcount_retain_u(&message->references) == DVM_REFCOUNT_LIVE;
}


/* Consume one message owner. object may be freed here. Diagnostic counter uses
 * native atomics too; no access to message occurs after a nonfinal drop. */
static void drop(void *object)
{
    Message *message = object;
    DVM_RefcountResult result;
    if (order_expected)
    {
        check(message->value == order_expected, "reverse cleanup order");
        order_expected--;
    }
    result = dvm_refcount_release_u(&message->references);
    check(result != DVM_REFCOUNT_REFUSED, "drop live owner");
    if (result == DVM_REFCOUNT_LAST)
    {
        free(message);
        check(dvm_refcount_retain_u(&retired) == DVM_REFCOUNT_LIVE, "retirement counter");
    }
}

static const DVM_OwnerOps ops = {retain, drop}; /* Immutable callbacks, process lifetime. */


/* Check synchronous callback reentry before acquiring new reference. object is
 * already live; probe scope must reject mutation while busy. Return real retain. */
static int reentrant_retain(void *object)
{
    check(!dvm_owner_scope_close(probe_scope), "retain close reentry");
    check(!dvm_owner_scope_retain(probe_scope, &probe_entry, object, &ops), "retain reentry");
    return retain(object);
}


/* Verify drop cannot reopen/mutate its draining ledger. object is owned until
 * drop consumes it; entry storage remains private and zeroed after detach. */
static void reentrant_drop(void *object)
{
    if (probe)
    {
        check(!probe_entry.scope && !probe_entry.object, "detach precedes callback");
        check(!dvm_owner_scope_retain(probe_scope, &probe_entry, probe_object, &ops), "drop reentry");
        check(!dvm_owner_scope_close(probe_scope), "close reentry");
    }
    drop(object);
}

static const DVM_OwnerOps reentrant_ops = {reentrant_retain, reentrant_drop}; /* Owner-local test callbacks. */


/* Create one root reference in ordinary malloc storage. value is immutable;
 * return owned message or semantic failure. No separate allocator is needed. */
static Message *message_new(int value)
{
    Message *message = malloc(sizeof(*message));
    check(message != NULL, "native allocation");
    message->references = 1;
    message->value = value;
    return message;
}


/* Bind native context or fail. Returned context is owned, with no managed roots;
 * callers close all scopes and unbind before destroy. */
static DVM_NativeContext *context_new(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    check(context && dvm_native_context_bind(context), "context setup");
    return context;
}


/* Consume bound context after explicit scope cleanup. No caller retains it. */
static void context_drop(DVM_NativeContext *context)
{
    check(dvm_native_context_unbind(context), "unbind after close");
    check(dvm_native_context_destroy(context), "context destroy");
}


/* Borrow main-rooted message, retain two independent owners and publish barrier
 * readiness. Main keeps root until joins; neither raw argument nor alias count
 * alone acquires lifetime. Cleanup explicitly models normal thread epilogue. */
static void aliases(void *argument)
{
    Message *message = argument;
    DVM_NativeContext *context = context_new();
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry first = {0};
    DVM_OwnerEntry second = {0};
    check(dvm_owner_scope_open(&scope), "worker open");
    check(dvm_owner_scope_retain(&scope, &first, message, &ops), "worker first alias");
    check(dvm_owner_scope_retain(&scope, &second, message, &ops), "worker second alias");
    fvm_port_sync_lock(gate);
    ready++;
    fvm_port_sync_signal_all(gate);
    while (!go)
    {
        fvm_port_sync_wait(gate);
    }
    fvm_port_sync_unlock(gate);
    check(message->value == 42, "worker live payload");
    check(dvm_owner_scope_drop(&first), "worker nil assignment");
    check(dvm_owner_scope_close(&scope), "worker epilogue");
    check(!first.scope && !second.scope, "worker slots cleared");
    context_drop(context);
}


/* Consume exclusively published envelope. Acceptance moves its owner without
 * retain; creator may already have closed its ledger. Last scope close frees. */
static void receive(void *argument)
{
    DVM_OwnerTransfer *transfer = argument;
    DVM_NativeContext *context = context_new();
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    check(dvm_owner_scope_open(&scope), "receiver open");
    check(dvm_owner_scope_accept(&scope, &entry, transfer), "receiver accept");
    check(!transfer->object && !transfer->ops, "accept consumes envelope");
    check(((Message *)entry.object)->value == 77, "handoff payload alive");
    check(dvm_owner_scope_close(&scope), "receiver last release");
    context_drop(context);
}


/* Probe foreign affinity while main excludes any mutation/retirement of scope.
 * argument scope is borrowed immutable metadata until this worker joins. */
static void foreign(void *argument)
{
    DVM_NativeContext *context = context_new();
    DVM_OwnerScope *scope = argument;
    DVM_OwnerTransfer transfer = {0};
    check(!dvm_owner_scope_close(scope), "foreign close");
    check(!dvm_owner_scope_drop(scope->head), "foreign drop");
    check(!dvm_owner_scope_export(scope->head, &transfer), "foreign export");
    context_drop(context);
}


/* Exercise native ownership boundaries and real frees. No parameters; return 0
 * only after all threads/owners/contexts retire. No managed GC/JIT claim. */
int main(void)
{
    DVM_NativeContext *context = context_new();
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    DVM_OwnerEntry extra = {0};
    DVM_OwnerEntry ordered[3] = {{0}};
    DVM_OwnerTransfer transfer = {0};
    DVM_ObjectDomain *domain;
    DVM_ObjectGuard guard = {0};
    FVM_PortThread *threads[WORKERS];
    Message *message;
    int index;
    retired = 1;
    check(dvm_owner_scope_open(&scope), "main open");
    check(!dvm_owner_scope_open(&scope), "double open");
    check(!dvm_native_context_unbind(context), "open scope blocks unbind");
    message = message_new(42);
    check(dvm_owner_scope_retain(&scope, &entry, message, &ops), "main distinct owner");
    check(dvm_refcount_load_u(&message->references) == 2, "retain increments count");
    check(!dvm_owner_scope_retain(&scope, &entry, message, &ops), "occupied slot unchanged");
    check(dvm_owner_scope_export(&entry, &transfer), "export owner");
    check(!entry.scope && !entry.object && transfer.object == message, "export empties source");
    check(dvm_owner_scope_accept(&scope, &entry, &transfer), "same owner adoption");
    check(!transfer.object && !transfer.ops, "adoption empties transfer");
    check(!dvm_owner_scope_accept(&scope, &extra, &transfer), "double accept");
    threads[0] = fvm_port_thread_start(foreign, &scope);
    check(threads[0] != NULL, "foreign thread start");
    fvm_port_thread_join(threads[0]);
    domain = dvm_object_domain_create();
    check(domain && dvm_object_guard_init(&guard, domain), "guard init");
    check(dvm_object_guard_try_enter(&guard) == DVM_OBJECT_ACQUIRED, "guard enter");
    check(!dvm_owner_scope_close(&scope) && entry.object == message, "guarded close unchanged");
    check(dvm_object_guard_leave(&guard), "guard leave");
    check(dvm_object_guard_fini(&guard) && dvm_object_domain_destroy(domain), "guard retire");
    check(dvm_owner_scope_close(&scope), "main close");
    check(dvm_refcount_load_u(&message->references) == 1, "close consumes exactly one");
    check(!entry.scope && !entry.object && !dvm_owner_scope_close(&scope), "closed empty slots");
    check(dvm_owner_scope_open(&scope), "reopen");
    probe_scope = &scope;
    probe_object = message;
    check(dvm_owner_scope_retain(&scope, &probe_entry, message, &reentrant_ops), "reentrant callback retain");
    probe = 1;
    check(dvm_owner_scope_close(&scope), "reentrant callback close");
    probe = 0;
    message->references = ULONG_MAX;
    check(dvm_owner_scope_open(&scope), "overflow open");
    check(!dvm_owner_scope_retain(&scope, &entry, message, &ops) && !entry.scope, "overflow no publication");
    message->references = 1;
    check(dvm_owner_scope_close(&scope), "overflow close");
    check(dvm_owner_scope_open(&scope), "zero open");
    message->references = 0;
    check(!dvm_owner_scope_retain(&scope, &entry, message, &ops) && !entry.scope, "zero no resurrection");
    message->references = 1;
    check(dvm_owner_scope_close(&scope), "zero close");
    check(dvm_owner_scope_open(&scope), "order open");
    for (index = 0; index < 3; index++)
    {
        Message *ordered_message = message_new(index + 1);
        check(dvm_owner_scope_retain(&scope, &ordered[index], ordered_message, &ops), "order retain");
        drop(ordered_message);
    }
    order_expected = 3;
    check(dvm_owner_scope_close(&scope) && order_expected == 0, "order close");
    gate = fvm_port_sync_create();
    check(gate != NULL, "barrier allocation");
    for (index = 0; index < WORKERS; index++)
    {
        threads[index] = fvm_port_thread_start(aliases, message);
        check(threads[index] != NULL, "alias thread start");
    }
    fvm_port_sync_lock(gate);
    while (ready != WORKERS)
    {
        fvm_port_sync_wait(gate);
    }
    check(dvm_refcount_load_u(&message->references) == 1 + 2 * WORKERS, "owners are references not threads");
    go = 1;
    fvm_port_sync_signal_all(gate);
    fvm_port_sync_unlock(gate);
    for (index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_refcount_load_u(&message->references) == 1, "all thread epilogues drop aliases");
    drop(message);
    fvm_port_sync_destroy(gate);
    for (index = 0; index < LIFETIMES; index++)
    {
        message = message_new(77);
        check(dvm_owner_scope_open(&scope), "handoff open");
        check(dvm_owner_scope_retain(&scope, &entry, message, &ops), "handoff retain");
        drop(message);
        check(dvm_owner_scope_export(&entry, &transfer), "handoff export");
        check(dvm_owner_scope_close(&scope), "sender closes before receiver");
        threads[0] = fvm_port_thread_start(receive, &transfer);
        check(threads[0] != NULL, "receiver start");
        fvm_port_thread_join(threads[0]);
    }
    message = message_new(77);
    check(dvm_owner_scope_open(&scope), "rollback open");
    check(dvm_owner_scope_retain(&scope, &entry, message, &ops), "rollback retain");
    drop(message);
    check(dvm_owner_scope_export(&entry, &transfer), "rollback export");
    check(dvm_owner_transfer_cancel(&transfer), "unpublished rollback drop");
    check(!dvm_owner_transfer_cancel(&transfer), "double cancel");
    check(dvm_owner_scope_close(&scope), "rollback close");
    check(dvm_refcount_load_u(&retired) == LIFETIMES + 6, "exact final frees");
    context_drop(context);
    puts("PASS owner scopes: 8 OS workers / 17 strong owners, 100 cross-thread frees, rollback and affinity");
    return 0;
}
