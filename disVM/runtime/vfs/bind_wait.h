/* Copyright (c) 2026 bhgv
 * EN: Host-only native bind admission for retained transactional resource plans.
 * RU: Host-only native bind admission для удерживаемых transactional resource plans.
 */
#ifndef DVM_VFS_BIND_WAIT_H
#define DVM_VFS_BIND_WAIT_H
#include "native_wait.h"
#include "path_cancel.h"
#include "stack.h"
#include "runtime_gc.h"


/* EN/RU: ACTIVE caller borrows retained source/target and cancellation output.
 * Run one native transaction; clear the source scope before managed reentry.
 * Return1 committed,0 stale,-1 refused; preserve success if cancellation loses commit.
 * ACTIVE caller заимствует source/target/cancelled; одна native transaction,
 * scope снимается до managed reentry;1 commit,0 stale,-1 refusal, без replay. */
static inline int dvm_bind_wait(DVM_ResourcePlan *source, DVM_ResourcePlan *target,
                                unsigned flags, int64_t index, int *cancelled)
{
    DVM_VfsWait *operation = NULL;
    *cancelled = 0;
    int released = dvm_capture_cancellable(source) && dvm_capture_cancellable(target) ?
        dvm_vfs_path_enter(&operation) : dvm_runtime_gc_native_try_leave();
    if (released < 0)
    {
        *cancelled = released == -2;
        return -1;
    }
    int result = dvm_stack_bind(source, target, flags, index);
    *cancelled = dvm_vfs_wait_end(operation);
    dvm_runtime_gc_native_enter(released);
    return result;
}
#endif
