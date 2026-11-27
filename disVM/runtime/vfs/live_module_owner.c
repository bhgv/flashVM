/* Copyright (c) 2026 bhgv
 * [GC:] EN: Copy the resident's private profile before preparing native routes.
 * RU: Скопировать частный профиль владельца перед подготовкой нативных маршрутов.
 */
#include "cap_internal.h"
#include "vfsnative_decl.h"
#ifdef DVM_TERMINAL_LAUNCH
#include "../terminal_launch/live_modules_private.h"
#include "guest.h"
#include "native_wait.h"
#include "publish_profile.h"
#include "runtime_gc.h"
#endif
#include <stddef.h>
#include <string.h>


/* [GC:] EN: Prepare routes for this exact resident. Return zero on refusal.
 * RU: Подготовить маршруты этого владельца. При отказе вернуть ноль. */
void Vfsnative_scalarliveprepare(void *frame)
{
    F_Vfsnative_scalarliveprepare call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarliveprepare, ret), sizeof(*call.ret));
    WORD result = 0;
#ifdef DVM_TERMINAL_LAUNCH
    Array *array = dvm_sys_owner(&call.profile);
    Prog *guest = currun();
    DVM_Namespace *group = dvm_namespace_current_group();
    unsigned char bytes[STYX_MOUNTED_EXPORT_LIMIT * STYX_SHAPED_OFFER_MAX];
    if (guest && group && array && (array == H || (array->len > 0 && (size_t)array->len <= sizeof(bytes))))
    {
        size_t size = array == H ? 0 : (size_t)array->len;
        if (size)
        {
            memcpy(bytes, array->data, size);
        }
        DVM_VfsWait *wait;
        int released = dvm_vfs_path_enter(&wait);
        if (released >= 0)
        {
            result = size ? dvm_live_module_prepare(group, guest->pid, bytes, size) :
                dvm_live_module_commit(guest->pid);
            result = !dvm_vfs_wait_end(wait) && result;
            dvm_runtime_gc_native_enter(released);
        }
    }
#endif
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}

/* EN: Start the session's internal listener loop.
 * RU: Запустить цикл внутреннего приёмника сессии. */
void Vfsnative_scalarlivelisten(void *frame)
{
    (void)frame;
#ifdef DVM_TERMINAL_LAUNCH
    Prog *guest = currun();
    if (guest)
    {
        dvm_live_modules_listen(guest->pid);
    }
#endif
}
