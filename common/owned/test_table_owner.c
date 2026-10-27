#include "table_owner.h"
#include <stdio.h>

typedef struct
{
    FvmTableOwner *owner; /* Shared table, kept alive until join. */
    uint64_t token;       /* Unique sequential worker capability. */
    int failed;           /* Result read after joining this worker. */
} Worker;


/* Read and increment under the outer transaction; no pointer escapes. */
static int increment(Fht *table, void *context)
{
    FhtKey key = {FHT_INDEX, 0, NULL, 0};
    int64_t value = 0;
    FhtStatus status = fht_get(table, key, &value);
    (void)context;
    if (status != FHT_OK && status != FHT_MISSING)
    {
        return 1;
    }
    return fht_set(table, key, value + 1) != FHT_OK;
}


/* Reenter the same owner through its public transaction boundary. */
static int nested(Fht *table, void *context)
{
    Worker *worker = context;
    int result;
    (void)table;
    if (fvm_table_owner_apply(worker->owner, worker->token, increment, NULL, &result))
    {
        return 1;
    }
    return result;
}


/* Execute 10000 competing read-modify-write transactions on a real Fht owner. */
static void *run(void *context)
{
    Worker *worker = context;
    int previous;
    int result;
    unsigned i;
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous))
    {
        worker->failed = 1;
        return NULL;
    }
    for (i = 0; i < 10000; i++)
    {
        if (fvm_table_owner_apply(worker->owner, worker->token, nested, worker, &result) || result)
        {
            worker->failed = 1;
            return NULL;
        }
    }
    return NULL;
}


/* Inspect final value under the same access policy as writers. */
static int verify(Fht *table, void *context)
{
    FhtKey key = {FHT_INDEX, 0, NULL, 0};
    int64_t value;
    (void)context;
    return fht_get(table, key, &value) != FHT_OK || value != 40000;
}


/* Return an application failure without throwing across owner cleanup. */
static int application_error(Fht *table, void *context)
{
    (void)table;
    (void)context;
    return 17;
}


/* Start/join native contenders; fail explicitly on lost updates or ownership errors. */
int main(void)
{
    FvmTableOwner owner;
    Worker workers[4];
    pthread_t threads[4];
    unsigned i;
    int previous;
    int result;
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous) || fvm_table_owner_init(&owner, NULL))
    {
        return 1;
    }
    for (i = 0; i < 4; i++)
    {
        workers[i].owner = &owner;
        workers[i].token = i + 1;
        workers[i].failed = 0;
        if (pthread_create(&threads[i], NULL, run, &workers[i]))
        {
            return 2;
        }
    }
    for (i = 0; i < 4; i++)
    {
        if (pthread_join(threads[i], NULL) || workers[i].failed)
        {
            return 3;
        }
    }
    if (fvm_table_owner_apply(&owner, 5, application_error, NULL, &result) || result != 17 ||
        fvm_table_owner_apply(&owner, 6, verify, NULL, &result) || result || fvm_table_owner_destroy(&owner))
    {
        return 4;
    }
    puts("PASS: nested guarded table transactions, no lost updates");
    return 0;
}
