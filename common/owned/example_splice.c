/* Build a protected table, then insert its own original prefix into itself. */
#include "table_owner.h"
#include <stdio.h>


/* Initialize values inside the owner's callback transaction.
 * table is borrowed only for this call; unused context owns no resources.
 * Return zero on success or one on allocation/semantic failure. */
static int populate(Fht *table, void *context)
{
    const int64_t values[] = {1, 9, 4}; /* Source sequence before self insertion. */
    unsigned i;
    (void)context;
    for (i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        FhtKey key = {FHT_INDEX, (int64_t)i, NULL, 0};
        if (fht_set(table, key, values[i]) != FHT_OK)
        {
            return 1;
        }
    }
    return 0;
}


/* Insert the pre-mutation prefix at an empty range, then verify every value.
 * The surrounding owner callback holds the guard for all reads and writes.
 * Self aliasing does not require another owner, and no payload pointer escapes. */
static int insert_self(Fht *table, void *context)
{
    const int64_t expected[] = {1, 1, 9, 4, 9, 4}; /* Prefix plus insertion and shifted tail. */
    unsigned i;
    int64_t value;
    (void)context;
    if (fht_splice(table, 1, 1, table) != FHT_OK || fht_length(table) != 6)
    {
        return 1;
    }
    for (i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        FhtKey key = {FHT_INDEX, (int64_t)i, NULL, 0};
        if (fht_get(table, key, &value) != FHT_OK || value != expected[i])
        {
            return 1;
        }
    }
    return 0;
}


/* Own lifetime explicitly; all error paths retire initialized storage.
 * Native cancellation stays disabled until the object is quiescent and retired. */
int main(void)
{
    FvmTableOwner owner;
    int previous;
    int result = 0;
    int failed = 0;
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous))
    {
        return 1;
    }
    if (fvm_table_owner_init(&owner, NULL))
    {
        pthread_setcancelstate(previous, NULL);
        return 1;
    }
    /* Tokens are local capabilities. This example executes them sequentially. */
    if (fvm_table_owner_apply(&owner, 1, populate, NULL, &result) || result ||
        fvm_table_owner_apply(&owner, 2, insert_self, NULL, &result) || result)
    {
        failed = 1;
    }
    if (fvm_table_owner_destroy(&owner))
    {
        failed = 1;
    }
    pthread_setcancelstate(previous, NULL);
    if (!failed)
    {
        puts("PASS self splice: [1, 9, 4] -> [1, 1, 9, 4, 9, 4]");
    }
    return failed;
}
