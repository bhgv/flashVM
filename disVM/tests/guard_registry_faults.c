/* Single-thread allocation ledger for actual registry/domain/port construction. */
#include "disvm/guard_registry.h"
#include "disvm/cell_access.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define RECORDS 256 /* Bounded fixture ledger, not a production object limit. */

static void *records[RECORDS]; /* Only allocations made while active, all retired before return. */
static unsigned int live; /* Single-thread fixture state; immutable during worker execution. */
static unsigned int attempts; /* Current constructor's native allocation attempts. */
static unsigned int refuse; /* One-based allocation to refuse; zero disables refusal. */
static int active; /* Ledger enabled only before TLS/worker startup. */
static int refused; /* Proves the requested allocation refusal actually occurred. */
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *value);


/* Semantic10 before allowing a leaked or untracked fixture allocation to escape. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL guard registry allocation: %s\n", message);
        exit(10);
    }
}


/* Called only before threaded work. Return1 to inject this exact allocation. */
static int deny(void)
{
    if (active)
    {
        attempts++;
        if (refuse && attempts == refuse)
        {
            refused = 1;
            return 1;
        }
    }
    return 0;
}


/* Record successfully allocated native storage without allocating metadata. */
static void record(void *value)
{
    if (active && value)
    {
        check(live < RECORDS, "bounded ledger");
        records[live++] = value;
    }
}


/* Intercept only linked native malloc references; no managed allocator is used. */
void *__wrap_malloc(size_t size)
{
    void *value;
    if (deny())
    {
        return NULL;
    }
    value = __real_malloc(size);
    record(value);
    return value;
}


/* Preserve real zeroing; refuse exactly one constructor/native node allocation. */
void *__wrap_calloc(size_t count, size_t size)
{
    void *value;
    if (deny())
    {
        return NULL;
    }
    value = __real_calloc(count, size);
    record(value);
    return value;
}


/* Check frees while ledger active. Other native fixture lifetime is unchanged. */
void __wrap_free(void *value)
{
    unsigned int index;
    if (active && value)
    {
        for (index = 0; index < live && records[index] != value; index++)
        {
        }
        check(index < live, "free belongs to constructor ledger");
        records[index] = records[--live];
    }
    __real_free(value);
}


/* Sweep every native constructor allocation to first complete success, then
 * reject a new-node allocation and prove the existing registry stays usable.
 * Run before thread creation; no wrapper state changes while workers execute. */
void dvm_test_guard_registry_faults(void)
{
    DVM_GuardRegistry *registry;
    DVM_GuardLease *lease;
    unsigned int index;
    int key = 0;
    active = 1;
    for (index = 1; index < RECORDS; index++)
    {
        refuse = index;
        attempts = 0;
        refused = 0;
        registry = dvm_guard_registry_create();
        if (registry)
        {
            check(!refused && attempts < refuse, "success only beyond injected allocation");
            check(dvm_guard_registry_destroy(&registry) && !registry && !live,
                "successful constructor fully retires");
            break;
        }
        check(refused && !live, "partial constructor fully retires");
    }
    check(index < RECORDS, "constructor allocation sweep completes");
    refuse = 0;
    registry = dvm_guard_registry_create();
    check(registry != NULL, "post-sweep registry");
    {
        unsigned int baseline = live;
        refuse = 1;
        attempts = 0;
        refused = 0;
        lease = dvm_guard_registry_acquire(registry, &key);
        check(!lease && refused && live == baseline && !dvm_guard_registry_entries(registry),
            "failed node preserves empty registry");
        refuse = 0;
        lease = dvm_guard_registry_acquire(registry, &key);
        check(lease && dvm_guard_registry_release(&lease) && live == baseline,
            "node success after refusal");
    }
    check(dvm_guard_registry_destroy(&registry) && !live, "final ledger empty");
    active = 0;
    printf("PASS guard registry allocations:%u constructor boundaries and node refusal, zero native ledger\n", index - 1);
}


/* Sweep actual cell-domain construction before any native competitors start.
 * Fixed guards allocate only at startup; completed begin/end uses no allocator. */
void dvm_test_cell_domain_faults(void)
{
    DVM_CellDomain *domain;
    unsigned int index;
    active = 1;
    for (index = 1; index < RECORDS; index++)
    {
        refuse = index;
        attempts = 0;
        refused = 0;
        domain = dvm_cell_domain_create();
        if (domain)
        {
            check(!refused && attempts < refuse, "cell success beyond injected allocation");
            check(dvm_cell_domain_destroy(&domain) && !domain && !live,
                "cell successful constructor fully retires");
            break;
        }
        check(refused && !live, "cell partial constructor fully retires");
    }
    check(index < RECORDS, "cell allocation sweep completes");
    active = 0;
    printf("PASS cell domain allocations:%u boundaries, zero native ledger\n", index - 1);
}


/* Enable fail-first ledger only around an already initialized cell interval.
 * Run on main before competitors; allocation during begin/end must be absent. */
void dvm_test_cell_noalloc_start(void)
{
    check(!active && !live, "clean no-allocation ledger start");
    active = 1;
    refuse = 1;
    attempts = 0;
    refused = 0;
}


/* Prove the guarded operation did not call a native allocator at all. */
void dvm_test_cell_noalloc_end(void)
{
    check(!attempts && !refused && !live, "cell interval allocates no native storage");
    active = 0;
}
