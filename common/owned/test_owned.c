#include "owned.h"
#include <errno.h>
#include <stdio.h>
#include <sched.h>

#define ITERATIONS 10000 /* Each native contender performs this many mutations. */

typedef struct
{
    FvmOwned guard; /* Protects value across all workers. */
    unsigned value; /* Deliberately non-atomic payload protected by guard. */
} Fixture;

typedef struct
{
    Fixture *fixture; /* Borrowed until native thread joins. */
    uint64_t token;   /* Distinct operation capability for this sequential worker. */
    int failed;       /* Written only by worker, read after join. */
} Worker;

/* Reenter during each payload mutation; report errors through joined worker. */
static void *increment(void *argument)
{
    Worker *worker = argument;
    unsigned i;
    int previous;
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous))
    {
        worker->failed = 1;
        return NULL;
    }
    for (i = 0; i < ITERATIONS; i++)
    {
        if (fvm_owned_enter(&worker->fixture->guard, worker->token) ||
            fvm_owned_enter(&worker->fixture->guard, worker->token))
        {
            worker->failed = 1;
            return NULL;
        }
        worker->fixture->value++;
        if (fvm_owned_leave(&worker->fixture->guard, worker->token) ||
            fvm_owned_leave(&worker->fixture->guard, worker->token))
        {
            worker->failed = 1;
            return NULL;
        }
    }
    return NULL;
}

/* Exercise real parking, recursive depth, diagnostics and wrong-owner rejection.
 * Fail explicitly; CTest timeout bounds any unexpected deadlock. */
int main(void)
{
    Fixture fixture;
    Worker workers[4];
    pthread_t threads[4];
    uint64_t owner;
    unsigned depth;
    unsigned waiters;
    unsigned i;
    int previous;
    fixture.value = 0;
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous) || fvm_owned_init(&fixture.guard) ||
        fvm_owned_enter(&fixture.guard, 99) || fvm_owned_enter(&fixture.guard, 99))
    {
        return 1;
    }
    if (fvm_owned_destroy(&fixture.guard) != EBUSY || fvm_owned_leave(&fixture.guard, 98) != EPERM)
    {
        return 2;
    }
    /* A failed cooperative probe must neither enqueue nor change ownership. */
    if (fvm_owned_try_enter(&fixture.guard, 98) != EBUSY ||
        fvm_owned_inspect(&fixture.guard, &owner, &depth, &waiters) || owner != 99 || depth != 2 || waiters != 0 ||
        fvm_owned_try_enter(&fixture.guard, 99) || fvm_owned_leave(&fixture.guard, 99))
    {
        return 9;
    }
    for (i = 0; i < 4; i++)
    {
        workers[i].fixture = &fixture;
        workers[i].token = i + 1;
        workers[i].failed = 0;
        if (pthread_create(&threads[i], NULL, increment, &workers[i]))
        {
            return 3;
        }
    }
    do
    {
        if (fvm_owned_inspect(&fixture.guard, &owner, &depth, &waiters))
        {
            return 4;
        }
        sched_yield();
    } while (waiters != 4);
    if (owner != 99 || depth != 2 || fixture.value != 0 || fvm_owned_leave(&fixture.guard, 99))
    {
        return 5;
    }
    if (fvm_owned_inspect(&fixture.guard, &owner, &depth, &waiters) || depth != 1 ||
        fvm_owned_leave(&fixture.guard, 99))
    {
        return 6;
    }
    for (i = 0; i < 4; i++)
    {
        if (pthread_join(threads[i], NULL) || workers[i].failed)
        {
            return 7;
        }
    }
    if (fixture.value != 4 * ITERATIONS || fvm_owned_destroy(&fixture.guard))
    {
        return 8;
    }
    puts("PASS: 40000 protected mutations, recursive ownership and parked diagnostics");
    return 0;
}
