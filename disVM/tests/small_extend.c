/* Actual canonical guard extension: bounded acquisition and continuous prefix. */
#include "disvm/small_range.h"
#include "disvm/refcount.h"
#include "cell_access_private.h"
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread int monitor; /* Allocation monitor belongs only to this OS owner. */
static __thread unsigned long allocations; /* Native calls made inside an access. */
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void __real_free(void *);


/* Count bytes requests only in the guarded test interval; forward actual malloc. */
void *__wrap_malloc(size_t size)
{
    if (monitor)
    {
        allocations++;
    }
    return __real_malloc(size);
}


/* Count allocation requests; count/size and returned storage follow libc. */
void *__wrap_calloc(size_t count, size_t size)
{
    if (monitor)
    {
        allocations++;
    }
    return __real_calloc(count, size);
}


/* Count retirement inside an access; value remains ordinary libc ownership. */
void __wrap_free(void *value)
{
    if (monitor)
    {
        allocations++;
    }
    __real_free(value);
}


/* Main owns domain/cells/counters through joined workers. Atomic positive
 * refcount words are fixture-only release/acquire handshake counters, not roots. */
typedef struct Shared
{
    DVM_CellDomain *domain; /* Creator keeps domain alive until every join. */
    unsigned long *a; /* Mapped aligned first native counter. */
    unsigned long *b; /* Mapped second counter, in a distinct canonical stripe. */
    unsigned long ready; /* Starts1; each opposed owner increments after claim. */
    unsigned long attempted; /* Starts1; each owner increments after try-extend. */
    unsigned long foreign; /* Starts1; foreign inspection completes before extension. */
    DVM_TripleAccess *accesses[2]; /* Borrowed stable live prefixes during opposed phase only. */
} Shared;


/* Immutable main-owned worker description until pthread_join returns. */
typedef struct Argument
{
    Shared *shared; /* Borrowed live fixture. */
    unsigned int id; /* Chooses the first counter independently of lock order. */
} Argument;


/* valid/message are a required predicate and borrowed diagnostic. Exit10 for
 * semantic controls before any unguarded data access or invalid cleanup. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL small extend: %s\n", message);
        exit(10);
    }
}


/* Create one bare native TLS context; no Dis heap permission is granted. */
static DVM_NativeContext *bind_owner(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    check(context && dvm_native_context_bind(context), "native owner bind");
    return context;
}


/* context is exclusively current and must have zero guards before retirement. */
static void retire(DVM_NativeContext *context)
{
    check(!dvm_native_context_object_count(context) && dvm_native_context_unbind(context) &&
        dvm_native_context_destroy(context), "native owner retire");
}


/* Bounded test-only atomic polling of an independent handshake; no OS park,
 * VM/world transition or callback while original guards are held. */
static void await(unsigned long *counter, unsigned long value)
{
    unsigned long attempts;
    for (attempts = 0; attempts < 100000000UL; attempts++)
    {
        if (dvm_refcount_load_u(counter) >= value)
        {
            return;
        }
    }
    check(0, "bounded native handshake");
}


/* Borrow immutable argument; force opposite guards before extension. Owner0
 * first acquires a free new stripe then encounters the other owner's stripe;
 * owner1 encounters a busy stripe immediately. Both must preserve their prefix. */
static void *opposed(void *value)
{
    Argument *argument = value;
    Shared *shared = argument->shared;
    DVM_NativeContext *context = bind_owner();
    DVM_TripleAccess access = {0};
    unsigned long *first = argument->id ? shared->b : shared->a;
    const void *next = argument->id ? (const void *)shared->a : (const unsigned char *)shared->b-1;
    DVM_ObjectGuard *original;
    monitor = 1;
    check(dvm_small_range_access_begin(shared->domain, first, sizeof(void *), first,
        sizeof(void *), first, sizeof(void *), &access) == DVM_CELL_ACQUIRED && access.count == 1,
        "opposed initial guard");
    original = access.guards[0];
    shared->accesses[argument->id] = &access;
    check(dvm_refcount_retain_u(&shared->ready) == DVM_REFCOUNT_LIVE, "publish opposed claim");
    await(&shared->ready, 3);
    check(dvm_small_range_access_extend(shared->accesses[1-argument->id], next,
        sizeof(void *)) == DVM_CELL_REFUSED && access.count == 1 && access.held == 1,
        "foreign extension unchanged");
    check(dvm_refcount_retain_u(&shared->foreign) == DVM_REFCOUNT_LIVE, "publish foreign result");
    await(&shared->foreign, 3);
    check(dvm_small_range_access_extend(&access, next, sizeof(void *)) == DVM_CELL_BUSY,
        "opposing extension busy");
    check(access.count == 1 && access.held == 1 && access.guards[0] == original &&
        !access.guards[1] && !access.guards[2] && dvm_native_context_object_count(context) == 1,
        "extension rollback prefix");
    check(dvm_refcount_retain_u(&shared->attempted) == DVM_REFCOUNT_LIVE, "publish opposed result");
    await(&shared->attempted, 3);
    check(dvm_small_range_access_end(&access), "opposed original end");
    monitor = 0;
    check(!allocations, "allocation-free extension");
    retire(context);
    return NULL;
}


/* Borrow argument; commit2000 paired updates only after both live cells are
 * excluded. Retry BUSY only after ending the entire original access, then yield. */
static void *increment(void *value)
{
    Argument *argument = value;
    Shared *shared = argument->shared;
    DVM_NativeContext *context = bind_owner();
    unsigned int completed = 0;
    while (completed < 2000)
    {
        DVM_TripleAccess access = {0};
        unsigned long *first = argument->id & 1 ? shared->b : shared->a;
        unsigned long *second = argument->id & 1 ? shared->a : shared->b;
        DVM_CellResult result;
        monitor = 1;
        result = dvm_small_range_access_begin(shared->domain, first, sizeof(void *),
            first, sizeof(void *), first, sizeof(void *), &access);
        if (result == DVM_CELL_ACQUIRED)
        {
            result = dvm_small_range_access_extend(&access, second, sizeof(void *));
            if (result == DVM_CELL_ACQUIRED)
            {
                check(access.count == 2 && access.held == 2 &&
                    dvm_native_context_object_count(context) == 2, "extended depth");
                check(*shared->a == *shared->b, "coherent pair");
                (*shared->a)++;
                (*shared->b)++;
                completed++;
            }
            else
            {
                check(result == DVM_CELL_BUSY, "parallel extension refusal");
            }
            check(dvm_small_range_access_end(&access), "parallel end");
        }
        else
        {
            check(result == DVM_CELL_BUSY, "parallel begin refusal");
        }
        monitor = 0;
        check(!allocations && !dvm_native_context_object_count(context), "parallel empty interval");
        if (result == DVM_CELL_BUSY)
        {
            sched_yield();
        }
    }
    retire(context);
    return NULL;
}


/* No arguments. Own real native domain/cells until all joins; verify capacity,
 * dedup, malformed extension, partial rollback and16000 coherent paired updates. */
int main(void)
{
    unsigned long cells[64] = {0};
    DVM_NativeContext *context = bind_owner();
    DVM_TripleAccess access = {0};
    DVM_TripleAccess empty = {0};
    Shared shared;
    Argument arguments[8];
    pthread_t threads[8];
    unsigned int index;
    unsigned int b_index = 16;
    const void *first = (unsigned char *)&cells[1]+1;
    const void *second = (unsigned char *)&cells[9]+1;
    void *third = (unsigned char *)&cells[17]+1;
    memset(&shared, 0, sizeof(shared));
    shared.domain = dvm_cell_domain_create();
    check(shared.domain != NULL && sizeof(unsigned long) == sizeof(void *), "native domain and width");
    monitor = 1;
    check(dvm_small_range_access_begin(shared.domain, first, sizeof(void *), second,
        sizeof(void *), third, sizeof(void *), &access) == DVM_CELL_ACQUIRED && access.count == 6,
        "six initial stripes");
    check(dvm_small_range_access_extend(&access, first, sizeof(void *)) == DVM_CELL_ACQUIRED &&
        access.count == 6 && access.held == 6, "deduplicated extension");
    check(dvm_small_range_access_extend(&access, &cells[33], sizeof(void *)) == DVM_CELL_REFUSED &&
        access.count == 6 && access.held == 6 && dvm_native_context_object_count(context) == 6,
        "capacity preserves prefix");
    check(dvm_small_range_access_extend(&access, NULL, sizeof(void *)) == DVM_CELL_REFUSED &&
        dvm_small_range_access_extend(&access, first, 0) == DVM_CELL_REFUSED &&
        dvm_small_range_access_extend(&access, first, sizeof(void *)+1) == DVM_CELL_REFUSED &&
        dvm_small_range_access_extend(&access, (void *)((uintptr_t)-1-1), sizeof(void *)) == DVM_CELL_REFUSED &&
        dvm_small_range_access_extend(&empty, first, sizeof(void *)) == DVM_CELL_REFUSED,
        "invalid extension unchanged");
    check(dvm_small_range_access_end(&access), "capacity end");
    monitor = 0;
    check(!allocations && dvm_refcount_load_u(&shared.domain->references) == 1,
        "single domain borrow restored");
    retire(context);
    if (((uintptr_t)&cells[b_index]/sizeof(void *)) % DVM_CELL_STRIPES == 0)
    {
        b_index++;
    }
    shared.a = &cells[0];
    shared.b = &cells[b_index];
    shared.ready = 1;
    shared.attempted = 1;
    shared.foreign = 1;
    for (index = 0; index < 8; index++)
    {
        arguments[index].shared = &shared;
        arguments[index].id = index;
    }
    for (index = 0; index < 2; index++)
    {
        check(!pthread_create(&threads[index], NULL, opposed, &arguments[index]), "start opposed owner");
    }
    for (index = 0; index < 2; index++)
    {
        check(!pthread_join(threads[index], NULL), "join opposed owner");
    }
    for (index = 0; index < 8; index++)
    {
        check(!pthread_create(&threads[index], NULL, increment, &arguments[index]), "start writer");
    }
    for (index = 0; index < 8; index++)
    {
        check(!pthread_join(threads[index], NULL), "join writer");
    }
    check(*shared.a == 16000 && *shared.b == 16000 &&
        dvm_refcount_load_u(&shared.domain->references) == 1, "complete paired updates and borrows");
    check(dvm_cell_domain_destroy(&shared.domain) && !shared.domain, "domain retirement");
    puts("PASS small extend: opposed/partial rollback,16000 pairs,eight OS writers,no allocation");
    return 0;
}
