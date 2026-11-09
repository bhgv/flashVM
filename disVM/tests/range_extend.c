/* Copyright (c) 2026 bhgv
 *
 * EN: Native range guard tests covering reuse, contention and rollback.
 * RU: Проверки повторного захвата, конкуренции и отката блокировок диапазонов.
 */
/* Actual canonical guard extension: bounded acquisition and continuous prefix. */
#include "disvm/range_access.h"
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
    DVM_RangeAccess *accesses[2]; /* Borrowed stable live prefixes during opposed phase only. */
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
        fprintf(stderr, "FAIL range extend: %s\n", message);
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


/* EN: Write the borrowed fixture's cell from a separate bound context.
 * RU: Записать поле заимствованного fixture из другого нативного контекста. */
static void *watch_write(void *fixture)
{
    Shared *shared = fixture;
    DVM_NativeContext *context = bind_owner();
    DVM_CellAccess access = {0};
    check(dvm_cell_access_begin(shared->domain, shared->a, shared->a, &access) == DVM_CELL_ACQUIRED,
        "foreign watch writer enters");
    (*shared->a)++;
    check(dvm_cell_access_end(&access), "foreign watch writer leaves");
    retire(context);
    return NULL;
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
    DVM_RangeAccess access = {0};
    unsigned long *first = argument->id ? shared->b : shared->a;
    const void *next = argument->id ? (const void *)shared->a : (const unsigned char *)shared->b-1;
    DVM_ObjectGuard *original;
    monitor = 1;
    check(dvm_range_access_three_begin(shared->domain, first, sizeof(void *), first,
        sizeof(void *), first, sizeof(void *), &access) == DVM_CELL_ACQUIRED && access.count == 1,
        "opposed initial guard");
    original = access.guards[0];
    shared->accesses[argument->id] = &access;
    check(dvm_refcount_retain_u(&shared->ready) == DVM_REFCOUNT_LIVE, "publish opposed claim");
    await(&shared->ready, 3);
    check(dvm_range_access_extend(shared->accesses[1-argument->id], next,
        sizeof(void *)) == DVM_CELL_REFUSED && access.count == 1 && access.held == 1,
        "foreign extension unchanged");
    check(dvm_refcount_retain_u(&shared->foreign) == DVM_REFCOUNT_LIVE, "publish foreign result");
    await(&shared->foreign, 3);
    check(dvm_range_access_extend(&access, next, sizeof(void *)) == DVM_CELL_BUSY,
        "opposing extension busy");
    check(access.count == 1 && access.held == 1 && access.guards[0] == original &&
        !access.guards[1] && !access.guards[2] && dvm_native_context_object_count(context) == 1,
        "extension rollback prefix");
    check(dvm_refcount_retain_u(&shared->attempted) == DVM_REFCOUNT_LIVE, "publish opposed result");
    await(&shared->attempted, 3);
    check(dvm_range_access_end(&access), "opposed original end");
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
        DVM_RangeAccess access = {0};
        unsigned long *first = argument->id & 1 ? shared->b : shared->a;
        unsigned long *second = argument->id & 1 ? shared->a : shared->b;
        DVM_CellResult result;
        monitor = 1;
        result = dvm_range_access_three_begin(shared->domain, first, sizeof(void *),
            first, sizeof(void *), first, sizeof(void *), &access);
        if (result == DVM_CELL_ACQUIRED)
        {
            result = dvm_range_access_extend(&access, second, sizeof(void *));
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
            check(dvm_range_access_end(&access), "parallel end");
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


/* EN: Hold the original writer until the test publishes its reservation.
 * RU: Удерживать исходную запись до публикации резервации в тесте. */
static void *prior_writer(void *value)
{
    Shared *shared = value;
    DVM_NativeContext *context = bind_owner();
    DVM_CellAccess access = {0};
    check(dvm_cell_access_begin(shared->domain, shared->a, shared->b, &access) == DVM_CELL_ACQUIRED,
        "writer entered before publication");
    dvm_refcount_retain_u(&shared->ready);
    while (dvm_refcount_load_u(&shared->attempted) == 1)
    {
        sched_yield();
    }
    *shared->b = 42;
    check(dvm_cell_access_end(&access), "prior writer releases");
    retire(context);
    return NULL;
}


/* EN: Check that the claim phase refuses only reads touching reserved fields.
 * RU: Проверить, что захват счётчиков отклоняет чтение лишь зарезервированных полей. */
static void *claim_reader(void *value)
{
    Shared *shared = value;
    DVM_NativeContext *context = bind_owner();
    DVM_RangeAccess range = {0};
    DVM_CellAccess cell = {0};
    check(dvm_cell_access_begin(shared->domain, shared->b, shared->a, &cell) == DVM_CELL_BUSY,
        "reserved source retries during claim");
    check(dvm_range_access_begin(shared->domain, shared->b, sizeof(void *), NULL, 0, &range) ==
        DVM_CELL_BUSY, "reserved range read retries during claim");
    check(dvm_range_access_begin(shared->domain, shared->b + 256, sizeof(void *), NULL, 0, &range) ==
        DVM_CELL_ACQUIRED, "unrelated reader on same stripe can enter");
    check(dvm_range_access_extend(&range, shared->b, sizeof(void *)) == DVM_CELL_BUSY &&
        range.count == 1 && dvm_range_access_end(&range), "refused read extension preserves prefix");
    retire(context);
    return NULL;
}


/* EN: Verify exact reservation of cells[9] in domain, including overlaps.
 * RU: Проверить точную резервацию cells[9] в domain, включая пересечения. */
static void check_reservation(DVM_CellDomain *domain, unsigned long *cells)
{
    void *fields[] = { &cells[9] };
    DVM_CellAccess cell = {0};
    DVM_RangeAccess range = {0};
    Shared shared = { .domain = domain, .a = &cells[8], .b = &cells[9], .ready = 1, .attempted = 1 };
    pthread_t writer;
    check(!pthread_create(&writer, NULL, prior_writer, &shared), "start prior writer");
    while (dvm_refcount_load_u(&shared.ready) == 1)
    {
        sched_yield();
    }
    const void *token = dvm_cell_reserve_fields(domain, fields, 1);
    check(token != NULL, "publish reservation");
    check(dvm_range_access_cells_begin(domain, fields, 1, &range) == DVM_CELL_BUSY,
        "collector cannot skip a prior writer");
    dvm_refcount_retain_u(&shared.attempted);
    check(!pthread_join(writer, NULL) && cells[9] == 42, "prior writer completed");
    check(dvm_range_access_cells_begin(domain, fields, 1, &range) == DVM_CELL_ACQUIRED &&
        dvm_range_access_end(&range), "drain earlier writers");
    check(dvm_cell_access_begin(domain, &cells[8], &cells[9], &cell) == DVM_CELL_BUSY,
        "reserved pointer write retries");
    check(dvm_range_access_begin(domain, NULL, 0, (char *)&cells[9] - 1, 2, &range) == DVM_CELL_BUSY,
        "partial overlap retries");
    check(dvm_range_access_begin(domain, &cells[9], sizeof(void *), NULL, 0, &range) == DVM_CELL_ACQUIRED &&
        dvm_range_access_end(&range), "reserved field remains readable");
    check(dvm_cell_access_begin(domain, &cells[9], &cells[265], &cell) == DVM_CELL_ACQUIRED &&
        dvm_cell_access_end(&cell), "unrelated write on same stripe");
    dvm_cell_reservation_claim(domain, token);
    check(!pthread_create(&writer, NULL, claim_reader, &shared) && !pthread_join(writer, NULL),
        "check claim phase from foreign context");
    check(dvm_range_access_cells_begin(domain, fields, 1, &range) == DVM_CELL_ACQUIRED &&
        dvm_range_access_end(&range), "claim owner can drain reserved field");
    dvm_cell_unreserve_fields(domain, token);
    check(dvm_cell_access_begin(domain, &cells[8], &cells[9], &cell) == DVM_CELL_ACQUIRED,
        "write proceeds after release");
    cells[9] = 0;
    check(dvm_cell_access_end(&cell), "release restored field");
}


/* No arguments. Own real native domain/cells until all joins; verify full-domain/empty access,
 * dedup, malformed extension, partial rollback and16000 coherent paired updates. */
int main(void)
{
    unsigned long cells[512] = {0};
    DVM_NativeContext *context = bind_owner();
    DVM_RangeAccess access = {0};
    DVM_RangeAccess empty = {0};
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
    check(dvm_range_access_three_begin(shared.domain, first, sizeof(void *), second,
        sizeof(void *), third, sizeof(void *), &access) == DVM_CELL_ACQUIRED && access.count == 6,
        "six initial stripes");
    check(dvm_range_access_extend(&access, first, sizeof(void *)) == DVM_CELL_ACQUIRED &&
        access.count == 6 && access.held == 6, "deduplicated extension");
    {
        const void *addition = NULL;
        unsigned int cell;
        for (cell = 0; cell < 512 && !addition; cell++)
        {
            unsigned int key = (unsigned int)(((uintptr_t)&cells[cell]/sizeof(void *))%DVM_CELL_STRIPES);
            DVM_ObjectGuard *candidate = &shared.domain->stripes[key];
            unsigned int prior;
            for (prior = 0; prior < access.count && access.guards[prior] != candidate; prior++)
            {
            }
            if (prior == access.count && candidate < access.guards[access.count-1])
            {
                addition = &cells[cell];
            }
        }
        check(addition && dvm_range_access_extend(&access, addition, sizeof(void *)) == DVM_CELL_ACQUIRED &&
            access.count == 7 && access.held == 7 && dvm_native_context_object_count(context) == 7,
            "extended depth");
        for (cell = 1; cell < access.count; cell++)
        {
            check(access.guards[cell-1] < access.guards[cell], "sorted extended metadata");
        }
    }
    check(dvm_range_access_extend(&access, first, sizeof(void *)+1) == DVM_CELL_ACQUIRED &&
        access.count == 7 && dvm_range_access_extend(&access, NULL, 0) == DVM_CELL_ACQUIRED,
        "generic spans and empty extension");
    check(dvm_range_access_extend(&access, NULL, sizeof(void *)) == DVM_CELL_REFUSED &&
        dvm_range_access_extend(&access, (void *)((uintptr_t)-1-1), sizeof(void *)) == DVM_CELL_REFUSED &&
        dvm_range_access_extend(&empty, first, sizeof(void *)) == DVM_CELL_REFUSED &&
        access.count == 7 && access.held == 7, "invalid extension unchanged");
    check(dvm_range_access_end(&access), "capacity end");
    check(dvm_range_access_three_begin(shared.domain, NULL, 0, NULL, 0, NULL, 0, &empty) == DVM_CELL_ACQUIRED &&
        !empty.count && dvm_range_access_extend(&empty, (void *)(uintptr_t)-1, 0) == DVM_CELL_ACQUIRED &&
        dvm_range_access_extend(&empty, &cells[0], sizeof(void *)) == DVM_CELL_ACQUIRED &&
        empty.count == 1 && dvm_range_access_end(&empty), "live empty access extension");
    check(dvm_range_access_three_begin(shared.domain, cells, sizeof(cells), NULL, 0, NULL, 0, &access) == DVM_CELL_ACQUIRED &&
        access.count == DVM_RANGE_GUARDS && dvm_range_access_extend(&access, cells, sizeof(cells)) == DVM_CELL_ACQUIRED &&
        access.count == DVM_RANGE_GUARDS && dvm_range_access_end(&access), "full canonical domain extension");
    void *batch[] = {third, (void *)first, (void *)second, third};
    check(dvm_range_access_cells_begin(shared.domain, batch, 4, &access) == DVM_CELL_ACQUIRED &&
        access.count == 6 && access.held == 6 && dvm_range_access_end(&access),
        "unordered cell batch deduplicates overlapping stripes");
    DVM_RangePlan plan;
    check(dvm_range_plan_cells(&plan, batch, 4), "prepare reusable cell plan");
    batch[3] = NULL;
    check(dvm_range_access_cells_begin(shared.domain, batch, 4, &access) == DVM_CELL_REFUSED &&
        !access.count && !access.domain && !dvm_native_context_object_count(context),
        "invalid final batch cell leaves no claims");
    check(!dvm_range_plan_cells(&plan, batch, 4), "refuse invalid replacement plan");
    for (unsigned int repeat = 0; repeat < 2; repeat++)
    {
        check(dvm_range_plan_begin(shared.domain, &plan, &access) == DVM_CELL_ACQUIRED &&
            access.count == 6 && access.held == 6 && dvm_range_access_end(&access),
            "reuse original plan after refusal");
    }
    DVM_RangeWatch watch = {0};
    check(dvm_range_watch_begin(shared.domain, &watch) &&
        dvm_range_plan_begin(shared.domain, &plan, &access) == DVM_CELL_ACQUIRED &&
        dvm_range_watch_sample(&watch, &access) && dvm_range_access_end(&access) &&
        dvm_range_plan_begin(shared.domain, &plan, &access) == DVM_CELL_ACQUIRED &&
        dvm_range_watch_unchanged(&watch, &access) && dvm_range_access_end(&access),
        "observer releases preserve its observations");
    monitor = 0;
    shared.a = &cells[9];
    pthread_t writer;
    check(!pthread_create(&writer, NULL, watch_write, &shared) && !pthread_join(writer, NULL),
        "join foreign watch writer");
    monitor = 1;
    check(dvm_range_plan_begin(shared.domain, &plan, &access) == DVM_CELL_ACQUIRED &&
        !dvm_range_watch_unchanged(&watch, &access) && dvm_range_access_end(&access) &&
        dvm_range_watch_end(&watch), "foreign cell release invalidates sampled range");
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
    context = bind_owner();
    check_reservation(shared.domain, cells);
    retire(context);
    check(dvm_cell_domain_destroy(&shared.domain) && !shared.domain, "domain retirement");
    puts("PASS range extend: opposed/partial rollback,16000 pairs,eight OS writers,no allocation");
    return 0;
}
