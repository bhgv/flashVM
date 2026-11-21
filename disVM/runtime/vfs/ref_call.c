/* Copyright (c) 2026 bhgv
 * EN: Snapshot and conditionally copy back a scalar-only referenced Limbo ADT.
 * RU: Снять снимок и условно вернуть поля скалярного ADT Limbo по ссылке.
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
#include "managed_owner.h"
#include "thread_port.h"
#include <stdlib.h>
#include <string.h>

typedef struct RefCells
{
    WORD narrow; /* Signed int in one NC64 cell. */
    LONG wide; /* Signed big in one NC64 cell. */
} RefCells;


/* EN: Claim the live object and optional scalar return span together.
 * RU: Захватить живой объект и необязательную область скалярного результата. */
static void claim(RefCells *object, void *ret, DVM_RangeAccess *access)
{
    for (;;)
    {
        DVM_CellResult result = dvm_managed_ranges_begin(object, sizeof(*object),
            ret, ret ? sizeof(RefCells) : 0, access);
        if (result == DVM_CELL_ACQUIRED)
        {
            return;
        }
        if (result != DVM_CELL_BUSY || !dvm_native_context_poll())
        {
            error(exHeap);
        }
        fvm_port_thread_sleep_us(50);
    }
}


/* EN: Capture scalar fields under the same range guards used by Limbo writes.
 * RU: Скопировать скалярные поля под теми же guards, что и записи Limbo. */
static RefCells snapshot(RefCells *object)
{
    DVM_RangeAccess access = {0};
    claim(object, NULL, &access);
    RefCells result;
    memcpy(&result, object, sizeof(result));
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return result;
}


/* EN: Publish both result and copy-back only if the source cells still match.
 * RU: Записать результат и обновление только при совпадении исходных полей. */
static void commit_ref(RefCells *object, RefCells *ret, const RefCells *before, const StyxRefReply *reply)
{
    uintptr_t from = (uintptr_t)object, to = (uintptr_t)ret;
    if (from <= to ? to - from < sizeof(*object) : from - to < sizeof(*ret))
    {
        error("overlapping remote ref result");
    }
    DVM_RangeAccess access = {0};
    claim(object, ret, &access);
    int matches = object->narrow == before->narrow && object->wide == before->wide;
    if (matches)
    {
        const RefCells updated = {reply->updated.narrow, reply->updated.wide};
        const RefCells value = {reply->result.narrow, reply->result.wide};
        memcpy(object, &updated, sizeof(updated));
        memcpy(ret, &value, sizeof(value));
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    if (!matches)
    {
        error("remote ref changed during call");
    }
}


/* EN: Keep the managed ref rooted by its mapped frame across one native wait.
 * RU: Удержать ref через карту кадра на время одного нативного ожидания. */
void dvm_ref_entry(void *frame, Type *expected_type, DVM_ScalarFiles *files)
{
    DirectRefFrame call;
    if (!frame || frame == H || frame != (void *)DVM_R.FP || (uintptr_t)frame > (uintptr_t)DVM_R.SP ||
        (uintptr_t)DVM_R.SP - (uintptr_t)frame < sizeof(call))
    {
        error(exBounds);
    }
    Type *type;
    memcpy(&type, (unsigned char *)frame + offsetof(Frame, t), sizeof(type));
    if (type != expected_type)
    {
        error(exModule);
    }
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(DirectRefFrame, ret), sizeof(RefCells));
    RefCells *object = dvm_sys_owner(&call.object);
    if (object == H || D2H(object)->t->size != sizeof(*object) || D2H(object)->t->np)
    {
        error("invalid remote ref argument");
    }
    RefCells before = snapshot(object);
    DVM_Namespace *group = dvm_namespace_current_group();
    if (!group || before.narrow < INT32_MIN || before.narrow > INT32_MAX)
    {
        error("remote ref call not sent");
    }
    Prog *process = currun();
    uint32_t pid = process && process->pid > 0 ? (uint32_t)process->pid : 0;
    DVM_LaunchFiles *descriptors = process ? ((DVM_RuntimeProcess *)process)->launch_files : NULL;
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    if (released < 0)
    {
        error("remote ref call not sent");
    }
    StyxTuple argument = {.narrow = (int32_t)before.narrow, .wide = before.wide};
    StyxRefReply reply;
    int outcome = dvm_ref_function_invoke(files, group, descriptors, pid, &argument, &reply);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || outcome != 1)
    {
        error(cancelled ? "remote ref call cancelled" : outcome == 2 ? "remote scalar owner error" :
            outcome < 0 ? "remote ref call not sent" : "remote ref call outcome unknown");
    }
    commit_ref(object, call.ret, &before, &reply);
}
