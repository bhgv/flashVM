#include "root_batch.h"

/* Own both native keys until the group is cleared; no managed GC in this process. */
int main(void)
{
    int first = 1;
    int second = 2;
    void *keys[2] = {&first, &second};
    DVM_RootBatch *batch = dvm_runtime_root_batch_create(2);
    if (!batch)
    {
        return 1;
    }
    if (!dvm_runtime_root_batch_publish(batch, keys, 2))
    {
        (void)dvm_runtime_root_batch_destroy(&batch);
        return 2;
    }
    /* Use only bounded work here. Registration grants no ownership of keys. */
    if (!dvm_runtime_root_batch_clear(batch))
    {
        return 3;
    }
    return dvm_runtime_root_batch_destroy(&batch) ? 0 : 4;
}
