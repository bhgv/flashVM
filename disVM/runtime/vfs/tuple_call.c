/* Copyright (c) 2026 bhgv
 * EN: Native NC64 tuple entry for mounted Dis modules.
 * RU: Нативный вход кортежа NC64 для смонтированных модулей Dis.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "fns.h"
#include "kernel.h"
#include "launch_files.h"
#include "runqueue.h"
#include "scalar_direct.h"
#include "scalar_files.h"
#include "sys_capture.h"
#include "runtime_gc.h"
#include "native_wait.h"
#include "guest.h"
#include "raise.h"
#include <string.h>


/* EN: Copy a composed native frame, perform one Styx exchange and publish its result.
 * RU: Скопировать составной кадр, выполнить обмен Styx и записать результат. */
void dvm_tuple_entry(void *frame, Type *expected_type, DVM_ScalarFiles *files)
{
    DirectTupleFrame call;
    if (!frame || frame == H || frame != (void *)DVM_R.FP || (uintptr_t)frame > (uintptr_t)DVM_R.SP ||
        (uintptr_t)DVM_R.SP - (uintptr_t)frame < sizeof(call))
    {
        error(exBounds);
    }
    Type *type;
    memcpy(&type, (unsigned char *)frame + offsetof(Frame, t), sizeof(type));
    if (type && type != expected_type)
    {
        error(exModule);
    }
    WORD result[2];
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(DirectTupleFrame, ret), sizeof(result));
    Prog *process = currun();
    uint32_t pid = process && process->pid > 0 ? (uint32_t)process->pid : 0;
    DVM_LaunchFiles *descriptors = process ? ((DVM_RuntimeProcess *)process)->launch_files : NULL;
    DVM_Namespace *group = dvm_namespace_current_group();
    if (!group || call.narrow < INT32_MIN || call.narrow > INT32_MAX)
    {
        error("remote tuple call not sent");
    }
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    if (released < 0)
    {
        error("remote tuple call not sent");
    }
    StyxTuple argument = {.narrow = (int32_t)call.narrow, .wide = call.wide}, answer;
    int outcome = dvm_tuple_function_invoke(files, group, descriptors, pid, &argument, &answer);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || outcome != 1)
    {
        error(cancelled ? "remote tuple call cancelled" : outcome == 2 ? "remote scalar owner error" :
            outcome < 0 ? "remote tuple call not sent" : "remote tuple call outcome unknown");
    }
    result[0] = answer.narrow;
    result[1] = answer.wide;
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}



/* EN: Copy the pointer-free frame, exchange values, then publish the nested return.
 * RU: Скопировать кадр без указателей, обменять значения, затем выдать вложенный результат. */
void dvm_call_list_entry(void *frame, Type *expected_type, DVM_ScalarFiles *files)
{
    DirectCallListFrame call;
    const char *input_shape = files->dynamic ? files->shape.arguments : styxcall_list_args;
    const char *output_shape = files->dynamic ? files->shape.result : styxcall_list_result;
    StyxScalarLayout input, output;
    if (!styxscalar_layout(input_shape, &input) || !styxscalar_layout(output_shape, &output))
    {
        error(exModule);
    }
    size_t extent = offsetof(DirectCallListFrame, arguments) + input.size;
    if (!frame || frame == H || frame != (void *)DVM_R.FP || (uintptr_t)frame > (uintptr_t)DVM_R.SP ||
        (uintptr_t)DVM_R.SP - (uintptr_t)frame < extent || extent > sizeof(call) ||
        output.size > STYX_DYNAMIC_CELLS * sizeof(WORD))
    {
        error(exBounds);
    }
    Type *type;
    memcpy(&type, (unsigned char *)frame + offsetof(Frame, t), sizeof(type));
    if (type && type != expected_type)
    {
        error(exModule);
    }
    WORD cells[STYX_DYNAMIC_CELLS];
    if (output.size)
    {
        dvm_sys_capture(frame, &call, extent, offsetof(DirectCallListFrame, ret), output.size);
    }
    else
    {
        memcpy(&call, frame, extent);
    }
    StyxCallValues arguments, answer;
    styxscalar_unpack(&input, call.arguments, &arguments);
    Prog *process = currun();
    uint32_t pid = process && process->pid > 0 ? (uint32_t)process->pid : 0;
    DVM_LaunchFiles *descriptors = process ? ((DVM_RuntimeProcess *)process)->launch_files : NULL;
    DVM_Namespace *group = dvm_namespace_current_group();
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    if (released < 0)
    {
        error("remote argument-list call not sent");
    }
    int outcome = dvm_call_list_function_invoke(files, group, descriptors, pid, &arguments, &answer);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || outcome != 1)
    {
        error(cancelled ? "remote argument-list call cancelled" : outcome == 2 ? "remote scalar owner error" :
            outcome < 0 ? "remote argument-list call not sent" : "remote argument-list call outcome unknown");
    }
    if (answer.count != output.count)
    {
        error("invalid remote argument-list result width");
    }
    if (output.size)
    {
        styxscalar_pack(&output, &answer, cells);
        dvm_sys_scalar_result(cells, call.ret, output.size);
    }
}
