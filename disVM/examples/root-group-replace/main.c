/* Native-only root metadata example: never load these keys into a VM collector. */
#ifdef __cplusplus
extern "C"
{
#endif
#include "root_batch.h"
#include "root_registry.h"
#ifdef __cplusplus
}
#endif
#include <stdio.h>


/* Own a reservation and three native identity tokens through final clear.
 * No CLI arguments, VM Heap, collector, reference transfer or callback is used.
 * Return0 after sparse/duplicate/empty replacement,1 after balanced error cleanup.
 * A hosted module must instead use its real Heap roots and independent admission;
 * this separate-process example demonstrates only metadata visibility. */
int main(void)
{
    int first = 1;
    int second = 2;
    int third = 3;
    void *old_keys[3];
    void *new_keys[3];
    DVM_RootBatch *batch = dvm_runtime_root_batch_create(3);
    old_keys[0] = old_keys[1] = &first;
    old_keys[2] = &second;
    new_keys[0] = new_keys[2] = &second;
    new_keys[1] = &third;
    if (!batch)
    {
        fputs("Unable to reserve root metadata\n",stderr);
        return 1;
    }
    /* Publication creates three GC-registration occurrences, not three owners. */
    if (!dvm_runtime_root_batch_publish(batch,old_keys,3))
    {
        goto fail;
    }
    /* One mutex interval removes the old group and installs the new multiset.
     * All three independently owned native tokens remain alive throughout. */
    if (!dvm_runtime_root_batch_replace(batch,new_keys,3) ||
        dvm_runtime_root_count(&first) != 0 || dvm_runtime_root_count(&second) != 2 ||
        dvm_runtime_root_count(&third) != 1)
    {
        goto fail;
    }
    /* Index1 now means the new vector's third-token occurrence. */
    if (!dvm_runtime_root_batch_remove(batch,1) || dvm_runtime_root_count(&third) != 0)
    {
        goto fail;
    }
    /* Empty replacement withdraws all occurrences but preserves active state.
     * Clear explicitly ends publication before the reservation can be destroyed. */
    if (!dvm_runtime_root_batch_replace(batch,NULL,0) ||
        dvm_runtime_root_batch_destroy(&batch) || !dvm_runtime_root_batch_clear(batch) ||
        !dvm_runtime_root_batch_destroy(&batch))
    {
        goto fail;
    }
    puts("PASS root-group replace: duplicates, new indices, active empty, cleanup");
    return 0;
fail:
    fputs("Root-group example failed\n",stderr);
    if (batch)
    {
        (void)dvm_runtime_root_batch_clear(batch);
        (void)dvm_runtime_root_batch_destroy(&batch);
    }
    return 1;
}
