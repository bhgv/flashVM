/* Shared mutable native object. No managed Heap or cross-VM pointer is exposed. */
#include "disvm/object_guard.h"
#include "disvm/owner_scope.h"
#include "disvm/refcount.h"
#include "disvm/native_context.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>

#define WORKERS 4 /* Native OS workers in this example, not an engine limit. */
#define UPDATES 1000 /* Protected increments per worker. */


/* The caller's root keeps storage alive until joined result inspection. */
typedef struct Counter
{
    unsigned long references; /* One strong owner for main plus each handed-off worker. */
    DVM_ObjectGuard guard;     /* Native inline metadata, initialized before publication. */
    int value;                /* Mutable payload, accessed under guard while workers run. */
} Counter;


/* Consume one strong reference. LAST has no other borrowers: waiters also own
 * their handed-off references. Guard retirement precedes freeing its storage. */
static void counter_drop(void *object)
{
    Counter *counter = object;
    DVM_RefcountResult result = dvm_refcount_release_u(&counter->references);
    if (result == DVM_REFCOUNT_REFUSED)
    {
        fputs("invalid native counter reference\n", stderr);
        exit(1);
    }
    if (result == DVM_REFCOUNT_LAST)
    {
        if (!dvm_object_guard_fini(&counter->guard))
        {
            fputs("retiring a held or waited guard\n", stderr);
            exit(1);
        }
        free(counter);
    }
}


/* Borrow already-owned native counter, retain a DISTINCT owner before handoff.
 * Return nonzero only for one successful retain; no payload access/allocation. */
static int counter_retain(void *object)
{
    Counter *counter = object;
    return dvm_refcount_retain_u(&counter->references) == DVM_REFCOUNT_LIVE;
}

static const DVM_OwnerOps counter_ops = {counter_retain, counter_drop}; /* Stable callbacks for every worker scope. */


/* argument transfers one retained owner. Bare TLS has no managed roots/heap.
 * Every BUSY result waits only for availability, then retries actual acquisition.
 * Each critical section is nonthrowing, bounded and releases before callbacks,
 * safepoints, native lease transitions or dropping its strong object owner. */
static void update(void *argument)
{
    DVM_OwnerTransfer *transfer = argument;
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    Counter *counter;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    int index;
    if (!context || !dvm_native_context_bind(context))
    {
        if (context)
        {
            dvm_native_context_destroy(context);
        }
        fputs("native context setup failed\n", stderr);
        exit(1);
    }
    if (!dvm_owner_scope_open(&scope) || !dvm_owner_scope_accept(&scope, &entry, transfer))
    {
        fputs("native scope handoff failed\n", stderr);
        exit(1);
    }
    counter = entry.object;
    for (index = 0; index < UPDATES; index++)
    {
        for (;;)
        {
            DVM_ObjectResult result = dvm_object_guard_try_enter(&counter->guard);
            if (result == DVM_OBJECT_ACQUIRED)
            {
                break;
            }
            if (result != DVM_OBJECT_BUSY ||
                dvm_object_guard_wait_available(&counter->guard) != DVM_OBJECT_AVAILABLE)
            {
                fputs("native guard protocol failed\n", stderr);
                exit(1);
            }
        }
        counter->value++;
        if (!dvm_object_guard_leave(&counter->guard))
        {
            fputs("native guard release failed\n", stderr);
            exit(1);
        }
    }
    if (!dvm_owner_scope_close(&scope))
    {
        fputs("native worker references did not retire\n", stderr);
        exit(1);
    }
    if (!dvm_native_context_unbind(context) || !dvm_native_context_destroy(context))
    {
        fputs("native guard context did not retire\n", stderr);
        exit(1);
    }
}


/* Keep one root for quiescent result inspection; publish distinct worker owners
 * before thread creation. Failed creation rolls back only the unreceived owner,
 * then joins prior workers and retires the same object/domain without leaking. */
int main(void)
{
    DVM_NativeContext *context;
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    DVM_OwnerTransfer transfers[WORKERS] = {{0}};
    DVM_ObjectDomain *domain = dvm_object_domain_create();
    Counter *counter = calloc(1, sizeof(*counter));
    FVM_PortThread *threads[WORKERS];
    int started = 0;
    int index;
    int result;
    if (!domain || !counter)
    {
        if (domain)
        {
            dvm_object_domain_destroy(domain);
        }
        free(counter);
        return 1;
    }
    counter->references = 1;
    if (!dvm_object_guard_init(&counter->guard, domain))
    {
        free(counter);
        dvm_object_domain_destroy(domain);
        return 1;
    }
    context = dvm_native_context_create(sizeof(long), NULL);
    if (!context || !dvm_native_context_bind(context) || !dvm_owner_scope_open(&scope))
    {
        if (context)
        {
            if (dvm_native_context_current() == context)
            {
                dvm_native_context_unbind(context);
            }
            dvm_native_context_destroy(context);
        }
        counter_drop(counter);
        dvm_object_domain_destroy(domain);
        fputs("main native context setup failed\n", stderr);
        return 1;
    }
    for (index = 0; index < WORKERS; index++)
    {
        if (!dvm_owner_scope_retain(&scope, &entry, counter, &counter_ops))
        {
            break;
        }
        if (!dvm_owner_scope_export(&entry, &transfers[started]))
        {
            break;
        }
        threads[started] = fvm_port_thread_start(update, &transfers[started]);
        if (!threads[started])
        {
            dvm_owner_transfer_cancel(&transfers[started]);
            break;
        }
        started++;
    }
    for (index = 0; index < started; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    if (!dvm_owner_scope_close(&scope) || !dvm_native_context_unbind(context) ||
        !dvm_native_context_destroy(context))
    {
        fputs("main native scope did not retire\n", stderr);
        return 1;
    }
    /* All writers joined and main retains the only remaining owner. */
    result = counter->value == started * UPDATES && started == WORKERS ? 0 : 1;
    counter_drop(counter);
    if (!dvm_object_domain_destroy(domain))
    {
        return 1;
    }
    if (!result)
    {
        puts("PASS native object guard: four OS contexts, 4000 protected updates, complete retirement");
    }
    return result;
}
