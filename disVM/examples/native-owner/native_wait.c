/* Native failure still returns through the matching heap reentry. */
#include "native_wait.h"
#include <stdlib.h>


/* Validate current active ownership BEFORE reading the shared epoch. All inputs
 * are borrowed as declared in native_wait.h. Only the completed status is written
 * after resume; protocol failure after release is fatal, never a managed return. */
int dvm_example_native_wait(DVM_HeapOwner *owner, DVM_ExampleEpoch epoch,
    void *clock, DVM_ExampleOperation operation, void *data, int *status)
{
    DVM_HeapOwnerStats state;
    unsigned long before;
    int result;
    if (!epoch || !operation || !status ||
        !dvm_native_owner_statistics(owner,&state) ||
        state.phase != DVM_HEAP_ACTIVE || state.pending_progress)
    {
        return 0;
    }
    before = epoch(clock);
    if (!dvm_native_owner_suspend(owner,before))
    {
        return 0;
    }
    /* The operation owns only native data. A negative I/O-style result still
     * goes through resume; it is not an excuse to escape the ownership protocol. */
    result = operation(data);
    if (!dvm_native_owner_resume(owner))
    {
        abort();
    }
    /* The shared counter is sampled only after active admission is restored. */
    if (!dvm_native_owner_progress(owner,epoch(clock)))
    {
        abort();
    }
    *status = result;
    return 1;
}
