/* Native byte/cell cross-protocol exclusion; no managed Heap is forged. */
#include "disvm/range_access.h"
#include "disvm/small_range.h"
#include "thread_port.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dvm_test_cell_noalloc_start(void); /* Single-thread actual allocator monitor. */
void dvm_test_cell_noalloc_end(void); /* Require no allocation attempt during spans. */

#define THREADS 8 /* Joined native owners, no managed scheduler simulation. */
#define ROUNDS 2000 /* Each writer alternates the wide and small APIs. */


/* Alignment plus live mapped bytes for full-domain and wraparound selections. */
typedef union Storage
{
    void *alignment; /* Keep canonical stripe computation exact. */
    unsigned char bytes[812 * sizeof(void *)]; /* Borrowed through all native joins. */
} Storage;


/* Shared fixture stays alive independently of every access metadata record. */
typedef struct Fixture
{
    DVM_CellDomain *domain; /* Same domain for raw ranges and pointer/small cells. */
    unsigned char *first; /* Unaligned native first counter. */
    unsigned char *second; /* Unaligned paired counter. */
    unsigned char *third; /* Disjoint counter touched only by three-span owners. */
} Fixture;


/* Semantic10 before unsafe continuation; no signal/timeout control acceptance. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL range access: %s\n", message);
        exit(10);
    }
}


/* Bare native TLS owns metadata only; it grants no Dis heap access. */
static DVM_NativeContext *bind(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    check(context && dvm_native_context_bind(context), "native context");
    return context;
}


/* Guard depth must be zero before native context teardown. */
static void unbind(DVM_NativeContext *context)
{
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "context retirement");
}


/* Private/finished access state only; no cross-thread live inspection. */
static int zero(DVM_RangeAccess *access)
{
    unsigned int index;
    if (access->domain || access->context || access->count || access->held)
    {
        return 0;
    }
    for (index = 0; index < DVM_RANGE_GUARDS; index++)
    {
        if (access->guards[index])
        {
            return 0;
        }
    }
    return 1;
}


/* Alternate byte-range and exact scalar protocols over the SAME native data.
 * Retry only after BUSY fully rolls back; no native wait spans an access. */
static void write_pair(void *argument)
{
    Fixture *fixture = argument;
    DVM_NativeContext *context = bind();
    unsigned int index;
    for (index = 0; index < ROUNDS; index++)
    {
        DVM_CellAccess small = {0};
        DVM_RangeAccess wide = {0};
        DVM_TripleAccess compact = {0};
        DVM_CellResult result;
        uintptr_t first;
        uintptr_t second;
        uintptr_t third;
        int use_wide = index % 3;
        do
        {
            if (use_wide == 2)
            {
                result = dvm_small_range_access_begin(fixture->domain, fixture->first, sizeof(uintptr_t),
                    fixture->second, sizeof(uintptr_t), fixture->third, sizeof(uintptr_t), &compact);
            }
            else if (use_wide)
            {
                result = dvm_range_access_three_begin(fixture->domain, fixture->first, 20 * sizeof(void *),
                    fixture->second, sizeof(uintptr_t), fixture->third, sizeof(uintptr_t), &wide);
            }
            else
            {
                result = dvm_cell_access_spans_begin(fixture->domain, fixture->first, sizeof(uintptr_t),
                    fixture->second, sizeof(uintptr_t), &small);
            }
            if (result == DVM_CELL_BUSY)
            {
                check(!dvm_native_context_object_count(context), "busy leaves no guards");
                fvm_port_thread_sleep_us(1);
            }
        } while (result == DVM_CELL_BUSY);
        check(result == DVM_CELL_ACQUIRED, "mixed claim");
        memmove(&first, fixture->first, sizeof(first));
        memmove(&second, fixture->second, sizeof(second));
        check(second == first * 3, "mixed payload invariant");
        first++;
        second += 3;
        memmove(fixture->first, &first, sizeof(first));
        memmove(fixture->second, &second, sizeof(second));
        if (use_wide)
        {
            memmove(&third, fixture->third, sizeof(third));
            third++;
            memmove(fixture->third, &third, sizeof(third));
            if (use_wide == 2)
            {
                check(dvm_small_range_access_end(&compact), "compact ends cleanly");
            }
            else
            {
                check(dvm_range_access_end(&wide) && zero(&wide), "wide ends cleanly");
            }
        }
        else
        {
            check(dvm_cell_access_end(&small), "small ends cleanly");
        }
    }
    unbind(context);
}


/* Source controls hit serial predicates before competing payload writes.
 * Exact expected stripe membership is independent of the selection loop. */
int main(void)
{
    Storage storage = {0};
    Fixture fixture = {0};
    DVM_RangeAccess wide = {0};
    DVM_CellAccess small = {0};
    DVM_NativeContext *context = bind();
    FVM_PortThread *threads[THREADS];
    uintptr_t address = (uintptr_t)storage.bytes;
    uintptr_t first;
    uintptr_t second;
    unsigned int index;
    fixture.first = storage.bytes + (256 + (248 + 256 - (address / sizeof(void *)) % 256) % 256) * sizeof(void *) + 1;
    fixture.second = fixture.first + 3 * sizeof(void *);
    fixture.third = fixture.first + 30 * sizeof(void *);
    fixture.domain = dvm_cell_domain_create();
    check(fixture.domain != NULL, "domain creates");
    dvm_test_cell_noalloc_start();
    check(dvm_range_access_begin(fixture.domain, NULL, 0, (void *)(uintptr_t)-1, 0, &wide) == DVM_CELL_ACQUIRED &&
        !wide.count && !dvm_cell_domain_destroy(&fixture.domain), "empty access pins domain");
    check(dvm_range_access_end(&wide) && zero(&wide), "empty access ends");
    check(dvm_range_access_begin(fixture.domain, NULL, 1, fixture.first, 1, &wide) == DVM_CELL_REFUSED &&
        dvm_range_access_begin(fixture.domain, (void *)(uintptr_t)-2, 4, fixture.first, 1, &wide) == DVM_CELL_REFUSED &&
        zero(&wide), "invalid geometry refuses unchanged");
    check(dvm_cell_access_spans_begin(fixture.domain, fixture.first - 1 + sizeof(void *) - 1, 1,
        fixture.first - 1 + sizeof(void *) - 1, 1, &small) == DVM_CELL_ACQUIRED && small.count == 1,
        "one byte selects exact boundary");
    check(dvm_cell_access_end(&small), "single-byte end");
    check(dvm_range_access_begin(fixture.domain, fixture.first, 20 * sizeof(void *),
        fixture.second, sizeof(uintptr_t), &wide) == DVM_CELL_ACQUIRED && wide.count == 21,
        "wrapped wide selection exact");
    for (index = 1; index < wide.count; index++)
    {
        check((uintptr_t)wide.guards[index - 1] < (uintptr_t)wide.guards[index], "ascending union order");
    }
    check(dvm_range_access_end(&wide) && zero(&wide), "wide end clears metadata");
    check(dvm_range_access_begin(fixture.domain, fixture.first, 256 * sizeof(void *),
        NULL, 0, &wide) == DVM_CELL_ACQUIRED && wide.count == 256, "large span covers domain");
    check(dvm_range_access_end(&wide), "full-domain end");
    check(dvm_range_access_three_begin(fixture.domain, fixture.first, sizeof(uintptr_t),
        fixture.second, sizeof(uintptr_t), fixture.third, sizeof(uintptr_t), &wide) == DVM_CELL_ACQUIRED &&
        wide.count == 6, "three exact independent spans");
    check(dvm_range_access_end(&wide) && zero(&wide), "three-span end");
    check(dvm_range_access_three_begin(fixture.domain, fixture.first, sizeof(uintptr_t),
        fixture.first, sizeof(uintptr_t), fixture.first, sizeof(uintptr_t), &wide) == DVM_CELL_ACQUIRED &&
        wide.count == 2, "three aliased spans deduplicate");
    check(dvm_range_access_end(&wide), "three alias end");
    check(dvm_range_access_three_begin(fixture.domain, fixture.first, 1, NULL, 1,
        fixture.third, 1, &wide) == DVM_CELL_REFUSED && zero(&wide), "invalid middle span");
    {
        DVM_TripleAccess compact = {0};
        check(dvm_small_range_access_begin(fixture.domain, fixture.first, sizeof(uintptr_t),
            fixture.second, sizeof(uintptr_t), fixture.third, sizeof(uintptr_t), &compact) == DVM_CELL_ACQUIRED &&
            compact.count == 6, "compact exact independent spans");
        for (index = 1; index < compact.count; index++)
        {
            check((uintptr_t)compact.guards[index-1] < (uintptr_t)compact.guards[index], "compact ascending order");
        }
        check(dvm_small_range_access_end(&compact) && !compact.context && !compact.domain &&
            !compact.count && !compact.held, "compact zero end");
        check(dvm_small_range_access_begin(fixture.domain, fixture.first, sizeof(uintptr_t),
            fixture.first, sizeof(uintptr_t), fixture.first, sizeof(uintptr_t), &compact) == DVM_CELL_ACQUIRED &&
            compact.count == 2, "compact deduplicates aliases");
        check(dvm_small_range_access_end(&compact), "compact alias end");
        check(dvm_small_range_access_begin(fixture.domain, fixture.first, sizeof(uintptr_t)+1,
            fixture.second, 1, fixture.third, 1, &compact) == DVM_CELL_REFUSED && !compact.domain,
            "compact refuses oversized span");
    }
    dvm_test_cell_noalloc_end();
    for (index = 0; index < THREADS; index++)
    {
        threads[index] = fvm_port_thread_start(write_pair, &fixture);
        check(threads[index] != NULL, "native writer starts");
    }
    for (index = 0; index < THREADS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    memmove(&first, fixture.first, sizeof(first));
    memmove(&second, fixture.second, sizeof(second));
    check(first == THREADS * ROUNDS && second == first * 3, "joined exact values");
    memmove(&first, fixture.third, sizeof(first));
    check(first == THREADS * (ROUNDS - (ROUNDS + 2) / 3), "joined third-span values");
    check(dvm_cell_domain_destroy(&fixture.domain) && !fixture.domain, "complete domain retirement");
    unbind(context);
    puts("PASS range access:8 OS writers,16000 mixed byte/cell updates, exact empty/wrap/full/three selections");
    return 0;
}
