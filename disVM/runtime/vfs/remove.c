/* Copyright (c) 2026 bhgv
 * EN: Standard Sys.remove on one retained selection, with source-owned native cleanup.
 * RU: Стандартный Sys.remove по одному retained selection с source-owned native cleanup.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "kernel.h"
#include "runtime_gc.h"
#include "sys_capture.h"
#include "guest.h"
#include "resource.h"
#include "native_wait.h"
#include "path_cancel.h"

extern char *dvm_runtime_file_path(String *); /* Rooted guest input, validated before native selection. */


/* EN: Capture rooted frame before path conversion; remove once after native
 * release. Plan cleanup retains directory through cancellation/reentry/unwind.
 * Only scalar publication may retry after effects; no deletion replay.
 * RU: Захватить rooted frame до path conversion; удалить один раз вне managed
 * admission. Plan cleanup удерживает каталог при cancellation/reentry/unwind.
 * После эффекта повторяется только scalar publication, не удаление. */
void Sys_remove(void *frame)
{
    F_Sys_remove call;
    DVM_ResourcePlan plan;
    WORD result = -1;
    const char *path;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_remove, ret), sizeof(*call.ret));
    path = dvm_runtime_file_path(call.s);
    if (path && dvm_resource_plan(dvm_namespace_current_group(), path, 0, &plan))
    {
        DVM_VfsWait *wait = NULL;
        int released = dvm_path_cancellable(&plan) ? dvm_vfs_path_enter(&wait) : dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            FVM_Result status = dvm_resource_remove(&plan);
            int cancelled = dvm_vfs_wait_end(wait);
            dvm_runtime_gc_native_enter(released);
            if (status == FVM_OK)
            {
                result = 0;
            }
            else
            {
                kwerrstr(cancelled ? "namespace remove cancelled" : "cannot remove namespace entry");
            }
        }
        else
        {
            kwerrstr("namespace remove cancelled or unavailable");
        }
        dvm_resource_close(&plan);
    }
    else
    {
        kwerrstr("invalid namespace remove path");
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}
