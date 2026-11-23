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
#include "managed_ref.h"
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


/* EN: Decode the return, including the empty return from a void function.
 * RU: Декодировать результат, включая пустой возврат функции без результата. */
static int read_result(const StyxCallShape *shape, const StyxReplyField *field,
    const void *slots, const FVM_SRecord *baseline, DVM_StyxValueFrame *result)
{
    if (!strcmp(shape->result, "()"))
    {
        return styxmanaged_is_nil(field->bytes, field->size);
    }
    if (strchr(shape->result, 'r'))
    {
        return dvm_styx_ref_result_read(shape, field->bytes, field->size, slots, baseline, result);
    }
    return dvm_styx_value_frame_read(STYX_DYNAMIC_REPLY_TAG, shape->result, field->bytes, field->size, result);
}


/* EN: Validate and commit the field reply from an opted-in owner.
 * RU: Проверить и применить изменения полей от владельца с поддержкой дельт. */
static int delta_reply(const StyxCallShape *shape, const unsigned char *bytes, size_t size,
    const DirectManagedFrame *call, const FVM_SRecord *baseline, int expand)
{
    StyxReplyField fields[2];
    StyxFieldDelta deltas[STYX_CALL_VALUES];
    DVM_StyxValueFrame result = {0};
    size_t count;
    unsigned char expanded[STYX_MANAGED_LIST_LIMIT];
    int valid = styxcall_reply_read(STYX_MANAGED_REPLY_TAG, bytes, size, fields) &&
        styxmanaged_deltas_base(shape->arguments, fields[1].bytes, fields[1].size, deltas, &count,
            expand ? baseline : NULL, expanded, sizeof(expanded)) &&
        read_result(shape, &fields[0], call->arguments, baseline, &result) &&
        dvm_styx_ref_delta_commit(shape->arguments, call->arguments, deltas, count, call->ret, &result);
    dvm_styx_value_frame_clear(&result);
    return valid;
}


/* [GC:] EN: Stage and validate both reply positions before touching live fields.
 * RU: Подготовить и проверить обе позиции ответа до изменения живых полей. */
static int managed_reply(const StyxCallShape *shape, const unsigned char *bytes, size_t size,
    const DirectManagedFrame *call, const FVM_SRecord *baseline)
{
    StyxReplyField fields[2];
    FVM_SRecord updates;
    char selected[STYX_SHAPE_MAX + 1], refs[STYX_SHAPE_MAX + 1];
    unsigned mask;
    void *objects[STYX_CALL_VALUES] = {0}, *chosen[STYX_CALL_VALUES];
    if (!styxcall_reply_read(STYX_MANAGED_REPLY_TAG, bytes, size, fields) ||
        !styxmanaged_updates(shape->arguments, fields[1].bytes, fields[1].size, selected, &mask, &updates) ||
        !dvm_styx_ref_slots(shape->arguments, call->arguments, refs, objects))
    {
        return 0;
    }
    size_t count = 0;
    for (size_t i = 0; i < STYX_CALL_VALUES; i++)
    {
        if (mask & (1U << i))
        {
            for (size_t j = 0; j < i; j++)
            {
                if (objects[i] == objects[j])
                {
                    return 0;
                }
            }
            chosen[count++] = objects[i];
        }
    }
    DVM_StyxValueFrame result = {0}, copyback = {0};
    unsigned char record[STYX_MANAGED_LIST_LIMIT];
    size_t extent;
    int valid = (!mask || (fvm_srecord_write(STYX_MANAGED_REF_TAG, updates.payload, updates.size,
        record, sizeof(record), &extent) == FVM_S_OK &&
        dvm_styx_value_frame_read(STYX_MANAGED_REF_TAG, selected, record, extent, &copyback))) &&
        read_result(shape, &fields[0], call->arguments, baseline, &result) &&
        dvm_styx_ref_commit(selected, chosen, &copyback, call->ret, &result);
    dvm_styx_value_frame_clear(&copyback);
    dvm_styx_value_frame_clear(&result);
    return valid;
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
    int graph = styxmanaged_graph_tag(files->managed_tag);
    DVM_CallChannels channels = {0};
    DVM_StyxRefTable refs = {.limit = STYX_CALL_REF_KEYS, .result = 1, .channels = &channels};
    unsigned long tag = graph ? files->managed_tag : STYX_MANAGED_REQUEST_TAG;
    int encoded = graph ?
        dvm_styx_graph_write(&refs, tag, files->shape.arguments, captured.arguments,
            inputs.size, request, sizeof(request), &request_size) :
        dvm_styx_value_frame_write(tag, files->shape.arguments, captured.arguments,
            inputs.size, request, sizeof(request), &request_size);
    unsigned char reply[STYX_MANAGED_LIST_LIMIT + 1];
    FVM_SRecord baseline;
    if (!encoded || !styxmanaged_list_read(tag, files->shape.arguments, request, request_size, &baseline))
    {
        dvm_call_channels_release(&channels);
        dvm_styx_ref_release(&refs, 0);
        error("cannot encode remote managed call");
    }
    baseline.tag = files->managed_tag;
    Prog *process = currun();
    uint32_t pid = process && process->pid > 0 ? (uint32_t)process->pid : 0;
    DVM_LaunchFiles *descriptors = process ? ((DVM_RuntimeProcess *)process)->launch_files : NULL;
    DVM_Namespace *group = dvm_namespace_current_group();
    DVM_VfsWait *wait;
    int released = group ? dvm_vfs_path_enter(&wait) : -1;
    if (released < 0)
    {
        dvm_call_channels_release(&channels);
        dvm_styx_ref_release(&refs, 0);
        error("remote managed call not sent");
    }
    channels.resolving = 1;
    int outcome = dvm_managed_function_exchange(files, group, descriptors, pid,
        &baseline, request, request_size, reply, sizeof(reply), &response_size, &channels, &released);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || outcome != 1)
    {
        dvm_call_channels_release(&channels);
        dvm_styx_ref_release(&refs, 0);
        error(cancelled ? "remote managed call cancelled" : outcome < 0 ?
            "remote managed call not sent" : "remote managed call outcome unknown");
    }
    int valid = graph ? dvm_styx_graph_reply_commit(&files->shape, &baseline, &refs, (const unsigned char *)frame + offsetof(DirectManagedFrame, arguments), captured.ret,
        reply, response_size) :
        files->managed_tag != STYX_MANAGED_REQUEST_TAG ? delta_reply(&files->shape, reply, response_size, &captured,
        &baseline, styxmanaged_has_snapshot(files->managed_tag)) :
        managed_reply(&files->shape, reply, response_size, &captured, &baseline);
    dvm_styx_ref_release(&refs, 0);
    dvm_call_channels_release(&channels);
    if (valid < 0)
    {
        error("remote ref argument changed while waiting");
    }
    if (valid)
    {
        return;
    }
    StyxScalarKind kind;
    int64_t code;
    if (styxscalarread(STYX_FUNCTION_REPLY_TAG, reply, response_size, &kind, &code) &&
        kind == STYX_SCALAR_ERROR)
    {
        error("remote scalar owner error");
    }
    error("invalid remote managed reply");
}


/* [GC:] EN: Serialize the return and publish one reply to the caller's fid.
 * RU: Кодировать результат и отправить один ответ для fid вызывающего. */
void dvm_styx_owner_finish(const StyxCallShape *shape, const void *arguments, const FVM_SRecord *baseline,
    const void *slots, size_t size, const Array *identities)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (!owner || !owner->remote_call_reply)
    {
        error("managed owner reply route unavailable");
    }
    unsigned char result[STYX_MANAGED_LIST_LIMIT];
    unsigned char reply[STYX_MANAGED_LIST_LIMIT];
    unsigned char updates[STYX_MANAGED_LIST_LIMIT];
    size_t result_size, update_size, reply_size;
    int valid = shape && baseline && (styxmanaged_graph_tag(baseline->tag) ?
        identities && identities != H &&
        dvm_styx_graph_reply_write(shape, baseline, (void *const *)identities->data, (size_t)identities->len,
            slots, size, reply, sizeof(reply), &reply_size) :
        ((!strcmp(shape->result, "()") ?
        fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, result, sizeof(result), &result_size) == FVM_S_OK :
        strchr(shape->result, 'r') ?
            dvm_styx_ref_result_write(shape, slots, size, arguments, baseline, result, sizeof(result), &result_size) :
            dvm_styx_value_frame_write(STYX_DYNAMIC_REPLY_TAG, shape->result, slots, size,
                result, sizeof(result), &result_size)) &&
        dvm_styx_ref_updates(shape->arguments, arguments, baseline, updates, sizeof(updates), &update_size) &&
        styxcall_reply_write(STYX_MANAGED_REPLY_TAG, result, result_size, updates, update_size,
            reply, sizeof(reply), &reply_size)));
    if (!valid)
    {
        if (!styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, reply, sizeof(reply), &reply_size))
        {
            dvm_scalar_caller_end((Prog *)owner);
            error("cannot encode managed owner refusal");
        }
    }
    dvm_scalar_owner_record(H, reply, reply_size, 1);
}
