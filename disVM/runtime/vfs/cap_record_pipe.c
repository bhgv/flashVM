/* Copyright (c) 2026 bhgv
 * EN: Expose a private native record pipe as a rooted hosted VFS capability.
 * RU: Выдать частный нативный канал записей через удерживаемое полномочие VFS.
 */
#include "cap_internal.h"
#include "record_pipe.h"


/* [GC:] EN: Create a fresh provider before publishing the rooted capability.
 * The return cell is captured from frame. Refusal returns nil.
 * RU: Создать новый провайдер до публикации удерживаемого полномочия.
 * Захватить ячейку результата из frame. При отказе вернуть nil. */
void Vfsexport_recordpipe(void *frame)
{
    F_Vfsexport_recordpipe call;
    DVM_CapState *state;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_recordpipe, ret), sizeof(*call.ret));
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (state && !(state->root = dvm_record_pipe()))
    {
        dvm_cap_abandon(value);
        value = H;
    }
    dvm_cap_publish(value, call.ret);
}
