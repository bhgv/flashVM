/* Copyright (c) 2026 bhgv
 * [GC:] EN: Dispatch compiled string and primitive-array calls through Styx.
 * RU: Передавать вызовы со строками и примитивными массивами через Styx.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "scalar_direct.h"
#include "scalar_files.h"
#include "managed_value.h"
#include "scalar_owner_internal.h"
#include "namespace.h"
#include "launch_files.h"
#include "runqueue.h"
#include "runtime_gc.h"
#include "native_wait.h"
#include "guest.h"
#include "raise.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/scalar_function.h"
#include <string.h>


/* [GC:] EN: Decode the successful two-position reply and reject copy-back until
 * an explicit update schema exists. Both positions are validated atomically.
 * RU: Разобрать успешный двухпозиционный ответ и пока отклонить copy-back
 * без отдельной схемы обновлений. Обе позиции проверяются целиком.
 */
static int managed_reply(const char *shape, const unsigned char *bytes, size_t size,
    DVM_StyxValueFrame *result)
{
    StyxReplyField fields[2];
    FVM_SRecord updates;
    size_t used;
    return styxcall_reply_read(STYX_MANAGED_REPLY_TAG, bytes, size, fields) &&
        (!strcmp(shape, "()") ? styxmanaged_is_nil(fields[0].bytes, fields[0].size) :
            dvm_styx_value_frame_read(STYX_DYNAMIC_REPLY_TAG, shape, fields[0].bytes, fields[0].size, result)) &&
        fvm_srecord_read(fields[1].bytes, fields[1].size, STYX_MANAGED_LIST_LIMIT, &updates, &used) == FVM_S_OK &&
        used == fields[1].size && updates.tag == STYX_MANAGED_ARRAY_TAG && !updates.size;
}


/* [GC:] EN: Capture mapped inputs, encode them before leaving GC admission,
 * exchange one ordinary function-fid call, then move decoded results into the
 * caller's mapped return area. No cross-heap pointer is serialized.
 * RU: Снять mapped аргументы, закодировать их до выхода из допуска GC,
 * обменять вызов через fid функции и перенести результат в mapped область.
 * Указатели между кучами не передаются.
 */
void dvm_managed_call_entry(void *frame, Type *expected_type, DVM_ScalarFiles *files)
{
    StyxScalarLayout inputs, outputs;
    size_t words;
    unsigned char pointers;
    if (!files || !files->dynamic || files->ref_profile ||
        !dvm_styx_value_shape_supported(files->shape.arguments) ||
        !dvm_styx_value_shape_supported(files->shape.result) ||
        !styxmanaged_layout(files->shape.arguments, &inputs) ||
        !styxmanaged_layout(files->shape.result, &outputs) ||
        !dvm_styx_frame_layout(files->shape.arguments, &words, &pointers) ||
        words > sizeof(((DirectManagedFrame *)0)->arguments) / sizeof(WORD) ||
        outputs.size > sizeof(((DVM_StyxValueFrame *)0)->words))
    {
        error(exModule);
    }
    size_t extent = offsetof(DirectManagedFrame, arguments) + words * sizeof(WORD);
    if (!frame || frame == H || frame != (void *)DVM_R.FP || (uintptr_t)frame > (uintptr_t)DVM_R.SP ||
        (uintptr_t)DVM_R.SP - (uintptr_t)frame < extent)
    {
        error(exBounds);
    }
    Type *actual_type;
    memcpy(&actual_type, (unsigned char *)frame + offsetof(Frame, t), sizeof(actual_type));
    if (!expected_type || actual_type != expected_type || expected_type->size != extent)
    {
        error(exModule);
    }
    DirectManagedFrame captured;
    memset(&captured, 0, sizeof(captured));
    memcpy(&captured, frame, extent);
    unsigned char request[STYX_MANAGED_LIST_LIMIT + 1];
    size_t request_size, response_size = 0;
    if (!dvm_styx_value_frame_write(STYX_MANAGED_REQUEST_TAG, files->shape.arguments,
        captured.arguments, inputs.size, request, sizeof(request), &request_size))
    {
        error("cannot encode remote managed call");
    }
    Prog *process = currun();
    uint32_t pid = process && process->pid > 0 ? (uint32_t)process->pid : 0;
    DVM_LaunchFiles *descriptors = process ? ((DVM_RuntimeProcess *)process)->launch_files : NULL;
    DVM_Namespace *group = dvm_namespace_current_group();
    DVM_VfsWait *wait;
    int released = group ? dvm_vfs_path_enter(&wait) : -1;
    if (released < 0)
    {
        error("remote managed call not sent");
    }
    int outcome = dvm_managed_function_exchange(files, group, descriptors, pid,
        request, request_size, sizeof(request), &response_size);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || outcome != 1)
    {
        error(cancelled ? "remote managed call cancelled" : outcome < 0 ?
            "remote managed call not sent" : "remote managed call outcome unknown");
    }
    DVM_StyxValueFrame result = {0};
    if (managed_reply(files->shape.result, request, response_size, &result))
    {
        if (!dvm_styx_value_frame_move(&result, captured.ret, outputs.size))
        {
            dvm_styx_value_frame_clear(&result);
            error(exHeap);
        }
        return;
    }
    StyxScalarKind kind;
    int64_t code;
    if (styxscalarread(STYX_FUNCTION_REPLY_TAG, request, response_size, &kind, &code) &&
        kind == STYX_SCALAR_ERROR)
    {
        error("remote scalar owner error");
    }
    dvm_styx_value_frame_clear(&result);
    error("invalid remote managed reply");
}


/* [GC:] EN: Serialize the return and publish one reply to the caller's fid.
 * RU: Кодировать результат и отправить один ответ для fid вызывающего. */
void dvm_styx_owner_finish(const char *shape, const void *slots, size_t size, int success)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (!owner || !owner->remote_call_reply)
    {
        error("managed owner reply route unavailable");
    }
    unsigned char result[STYX_MANAGED_LIST_LIMIT];
    unsigned char reply[STYX_MANAGED_LIST_LIMIT];
    unsigned char updates[16];
    size_t result_size, update_size, reply_size;
    if (!(success && shape && (!strcmp(shape, "()") ?
        fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, result, sizeof(result), &result_size) == FVM_S_OK :
        dvm_styx_value_frame_write(STYX_DYNAMIC_REPLY_TAG, shape, slots, size,
            result, sizeof(result), &result_size)) &&
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, NULL, 0, updates, sizeof(updates), &update_size) == FVM_S_OK &&
        styxcall_reply_write(STYX_MANAGED_REPLY_TAG, result, result_size, updates, update_size,
            reply, sizeof(reply), &reply_size)))
    {
        if (!styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, reply, sizeof(reply), &reply_size))
        {
            dvm_scalar_caller_end((Prog *)owner);
            error("cannot encode managed owner refusal");
        }
    }
    dvm_scalar_owner_record(H, reply, reply_size, 1);
}
