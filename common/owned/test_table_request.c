/* Real contention and failure recovery for the exact request layer used by DisVM. */
#include "table_request.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    FvmTableOwner *destination; /* Both owners outlive the joined worker. */
    FvmTableOwner *source;      /* Opposite direction exercises stable pair ordering. */
    uint64_t token;             /* Unique sequential worker capability. */
    int failed;                 /* Observed only after native join. */
} Worker;

typedef struct
{
    size_t calls; /* Allocation ordinal, accessed only under this payload's guard. */
    size_t fail;  /* One injected allocation refusal, zero when disabled. */
    size_t live;  /* Outstanding blocks, checked after owner destruction. */
} Allocations;


/* Allocate counted caller-owned storage, refusing one selected ordinal. */
static void *allocate(void *context, size_t size)
{
    Allocations *state = context;
    void *result;
    state->calls++;
    if (state->calls == state->fail)
    {
        return NULL;
    }
    result = malloc(size);
    if (result)
    {
        state->live++;
    }
    return result;
}


/* Retire one matching block; context outlives all owners/snapshots. */
static void release(void *context, void *memory)
{
    Allocations *state = context;
    if (memory)
    {
        state->live--;
        free(memory);
    }
}


/* Prepare one dense scalar through the same guarded boundary as VM builtins. */
static int seed(FvmTableOwner *owner, uint64_t token)
{
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_SET;
    request.key.kind = FHT_INDEX;
    request.value = 42;
    return fvm_table_request(owner, token, &request) || request.status != FHT_OK;
}


/* Perform reversed two-owner copies and guarded reads without global payload locks.
 * Cancellation stays disabled through work; errors are recorded for the joiner. */
static void *copy_worker(void *context)
{
    Worker *worker = context;
    unsigned i;
    int previous;
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous))
    {
        worker->failed = 1;
        return NULL;
    }
    for (i = 0; i < 2000; i++)
    {
        FvmTableRequest copy = {0};
        FvmTableRequest read = {0};
        copy.operation = FVM_TABLE_SPLICE;
        copy.end = -1;
        copy.replacement = worker->source;
        read.operation = FVM_TABLE_GET;
        read.key.kind = FHT_INDEX;
        if (fvm_table_request(worker->destination, worker->token, &copy) || copy.status != FHT_OK ||
            fvm_table_request(worker->destination, worker->token, &read) || read.status != FHT_OK || read.value != 42)
        {
            worker->failed = 1;
            return NULL;
        }
    }
    return NULL;
}


/* Check that a quiescent guard has no forgotten recursion level or waiting worker. */
static int idle(FvmOwned *guard)
{
    uint64_t owner;
    unsigned depth;
    unsigned waiters;
    return fvm_owned_inspect(guard, &owner, &depth, &waiters) || owner || depth || waiters;
}


/* Inject failures inside locked self-splices; require rollback, release and no leaks. */
static int allocation_failures(uint64_t token)
{
    Allocations state = {0, 0, 0};
    FhtAllocator allocator = {&state, allocate, release};
    FvmTableOwner owner;
    unsigned ordinal;
    if (fvm_table_owner_init(&owner, &allocator) || seed(&owner, token))
    {
        return 1;
    }
    for (ordinal = 1; ordinal < 30; ordinal++)
    {
        FvmTableRequest copy = {0};
        FvmTableRequest read = {0};
        copy.operation = FVM_TABLE_SPLICE;
        copy.end = -1;
        copy.replacement = &owner;
        state.fail = state.calls + ordinal;
        if (fvm_table_request(&owner, token, &copy) || (copy.status != FHT_OK && copy.status != FHT_NOMEM) ||
            idle(&owner.guard))
        {
            return 2;
        }
        read.operation = FVM_TABLE_GET;
        read.key.kind = FHT_INDEX;
        if (fvm_table_request(&owner, token, &read) || read.status != FHT_OK || read.value != 42)
        {
            return 3;
        }
    }
    if (fvm_table_owner_destroy(&owner) || state.live)
    {
        return 4;
    }
    return 0;
}


/* Exercise unique tokens, pair recursion, inversion rejection, contention and
 * application errors. Return nonzero explicitly; CTest bounds any deadlock. */
int main(void)
{
    FvmTableOwner left;
    FvmTableOwner right;
    FvmOwned *low;
    FvmOwned *high;
    Worker workers[4];
    pthread_t threads[4];
    uint64_t token;
    uint64_t wrong;
    unsigned i;
    int previous;
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous) || fvm_owned_token(&token) ||
        fvm_owned_token(&wrong) || !token || token == wrong || fvm_table_owner_init(&left, NULL) ||
        fvm_table_owner_init(&right, NULL) || seed(&left, token) || seed(&right, token))
    {
        return 1;
    }
    low = (uintptr_t)&left.guard < (uintptr_t)&right.guard ? &left.guard : &right.guard;
    high = low == &left.guard ? &right.guard : &left.guard;
    if (fvm_owned_enter_pair(high, low, token) || fvm_owned_enter_pair(low, high, token) ||
        fvm_owned_leave_pair(high, low, wrong) != EPERM || fvm_owned_leave_pair(high, low, token) ||
        fvm_owned_leave_pair(low, high, token) || idle(low) || idle(high))
    {
        return 2;
    }
    if (fvm_owned_enter(high, token) || fvm_owned_enter_pair(high, low, token) != EDEADLK ||
        fvm_owned_leave(high, token) || idle(low) || idle(high))
    {
        return 3;
    }
    for (i = 0; i < 4; i++)
    {
        workers[i].destination = i % 2 ? &left : &right;
        workers[i].source = i % 2 ? &right : &left;
        workers[i].failed = 0;
        if (fvm_owned_token(&workers[i].token) || pthread_create(&threads[i], NULL, copy_worker, &workers[i]))
        {
            return 4;
        }
    }
    for (i = 0; i < 4; i++)
    {
        if (pthread_join(threads[i], NULL) || workers[i].failed)
        {
            return 5;
        }
    }
    if (idle(low) || idle(high) || fvm_table_owner_destroy(&left) || fvm_table_owner_destroy(&right) ||
        allocation_failures(token))
    {
        return 6;
    }
    puts("PASS: ordered owner pairs, 8000 reversed copies, recursion, rollback and released guards");
    return 0;
}
