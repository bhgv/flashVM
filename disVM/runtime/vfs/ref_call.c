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


/* EN: Claim up to two rooted objects and a return span in one ordered union.
 * RU: Захватить до двух укоренённых объектов и результат в одном упорядоченном наборе.
 * Retry busy admission before any reads or writes. NULL and zero omit the result.
 * Повторять занятый захват до чтения или записи. NULL и ноль пропускают результат. */
static void claim(RefCells *first, RefCells *second, void *ret, size_t width, DVM_RangeAccess *access)
{
    for (;;)
    {
        DVM_CellResult result = dvm_managed_three_begin(first, sizeof(*first),
            second, second ? sizeof(*second) : 0, ret, width, access);
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


/* EN: Release an admitted exact-owner access or abort on broken invariants.
 * RU: Освободить захват текущего владельца либо остановиться при нарушении инварианта. */
static void release_claim(DVM_RangeAccess *access)
{
    if (!dvm_range_access_end(access))
    {
        abort();
    }
}


/* EN: Snapshot unique objects together and stage input cells after dropping guards.
 * RU: Снять общий снимок уникальных объектов и подготовить аргументы после снятия блокировок. */
static void snapshot(RefCells *const *objects, RefCells *before, StyxRefPair *call)
{
    DVM_RangeAccess access = {0};
    claim(objects[0], objects[1], NULL, 0, &access);
    for (size_t i = 0; i < call->count; i++)
    {
        memcpy(&before[i], objects[i], sizeof(before[i]));
    }
    release_claim(&access);
    for (size_t i = 0; i < call->count; i++)
    {
        if (before[i].narrow < INT32_MIN || before[i].narrow > INT32_MAX)
        {
            error("remote ref call not sent");
        }
        call->objects[i] = (StyxTuple){(int32_t)before[i].narrow, before[i].wide};
    }
}


/* EN: Publish result and every update together after comparing all old fields.
 * RU: Вместе выдать результат и все обновления после сверки исходных полей. */
static void commit_ref(RefCells *const *objects, void *ret, size_t width,
    const RefCells *before, const StyxRefPair *reply)
{
    for (size_t i = 0; width && i < reply->count; i++)
    {
        uintptr_t from = (uintptr_t)objects[i], to = (uintptr_t)ret;
        if (from <= to ? to - from < sizeof(RefCells) : from - to < width)
        {
            error("overlapping remote ref result");
        }
    }
    DVM_RangeAccess access = {0};
    claim(objects[0], objects[1], ret, width, &access);
    int matches = 1;
    for (size_t i = 0; i < reply->count; i++)
    {
        if (objects[i]->narrow != before[i].narrow || objects[i]->wide != before[i].wide)
        {
            matches = 0;
            break;
        }
    }
    if (matches)
    {
        for (size_t i = 0; i < reply->count; i++)
        {
            const RefCells updated = {reply->objects[i].narrow, reply->objects[i].wide};
            memcpy(objects[i], &updated, sizeof(updated));
        }
        if (width)
        {
            const RefCells value = {reply->result.narrow, reply->result.wide};
            memcpy(ret, &value, width);
        }
    }
    release_claim(&access);
    if (!matches)
    {
        error("remote ref changed during call");
    }
}


/* EN: Check that both managed pointers will stay rooted by the actual frame map.
 * RU: Проверить, что карта активного кадра удерживает управляемые указатели. */
static void capture_frame(void *frame, Type *expected_type, void *output,
    size_t size, size_t ret_offset, size_t width)
{
    if (!frame || frame == H || frame != (void *)DVM_R.FP || (uintptr_t)frame > (uintptr_t)DVM_R.SP ||
        (uintptr_t)DVM_R.SP - (uintptr_t)frame < size)
    {
        error(exBounds);
    }
    Type *type;
    memcpy(&type, (unsigned char *)frame + offsetof(Frame, t), sizeof(type));
    if (type != expected_type)
    {
        error(exModule);
    }
    if (width)
    {
        dvm_sys_capture(frame, output, size, ret_offset, width);
    }
    else
    {
        memcpy(output, frame, size);
    }
}


/* EN: Borrow a nonnil scalar-looking owner rooted in the caller frame.
 * RU: Заимствовать ненулевой объект со скалярными полями из корней кадра клиента. */
static RefCells *ref_object(void *const *slot)
{
    RefCells *object = dvm_sys_owner(slot);
    if (object == H || D2H(object)->t->size != sizeof(*object) || D2H(object)->t->np)
    {
        error("invalid remote ref argument");
    }
    return object;
}


/* EN: Hold the snapshot locally across one native wait and commit only a trusted reply.
 * RU: Удерживать снимок локально на время обмена и применить только проверенный ответ.
 * objects contains one or two distinct rooted objects. NULL second means one object.
 * objects содержит один или два разных объекта. NULL во второй позиции означает один. */
static void invoke(DVM_ScalarFiles *files, RefCells *const *objects, void *ret, size_t width, int paired)
{
    StyxRefPair value = {.count = objects[1] ? 2 : 1};
    RefCells before[2];
    snapshot(objects, before, &value);
    DVM_Namespace *group = dvm_namespace_current_group();
    Prog *process = currun();
    uint32_t pid = process && process->pid > 0 ? (uint32_t)process->pid : 0;
    DVM_LaunchFiles *descriptors = process ? ((DVM_RuntimeProcess *)process)->launch_files : NULL;
    DVM_VfsWait *wait;
    int released = group ? dvm_vfs_path_enter(&wait) : -1;
    if (released < 0)
    {
        error("remote ref call not sent");
    }
    StyxRefReply single;
    int outcome = paired ? dvm_ref_pair_invoke(files, group, descriptors, pid, &value) :
        dvm_ref_function_invoke(files, group, descriptors, pid, &value.objects[0], &single);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || outcome != 1)
    {
        error(cancelled ? "remote ref call cancelled" : outcome == 2 ? "remote scalar owner error" :
            outcome < 0 ? "remote ref call not sent" : "remote ref call outcome unknown");
    }
    if (!paired)
    {
        value.objects[0] = single.updated;
        value.result = single.result;
    }
    if (files->ref_profile && ((files->result_cells < 2 && value.result.wide) ||
        (!files->result_cells && value.result.narrow)))
    {
        error("invalid remote ref result padding");
    }
    commit_ref(objects, ret, width, before, &value);
}


/* EN: Keep one managed ref rooted by its compiled frame across the exchange.
 * RU: Удержать один ref через карту кадра во время обмена. */
void dvm_ref_entry(void *frame, Type *expected_type, DVM_ScalarFiles *files)
{
    DirectRefFrame call;
    size_t width = files->ref_profile ? files->result_cells * sizeof(WORD) : sizeof(RefCells);
    capture_frame(frame, expected_type, &call, sizeof(call), offsetof(DirectRefFrame, ret), width);
    RefCells *objects[] = {ref_object(&call.object), NULL};
    invoke(files, objects, width ? call.ret : NULL, width, 0);
}


/* EN: Assign call-local object IDs by first appearance and coalesce equal owners.
 * RU: Назначить номера по первому появлению и объединить две ссылки на один объект. */
void dvm_ref_pair_entry(void *frame, Type *expected_type, DVM_ScalarFiles *files)
{
    DirectRefPairFrame call;
    size_t width = files->ref_profile ? files->result_cells * sizeof(WORD) : sizeof(RefCells);
    capture_frame(frame, expected_type, &call, sizeof(call), offsetof(DirectRefPairFrame, ret), width);
    RefCells *first = ref_object(&call.objects[0]);
    RefCells *second = ref_object(&call.objects[1]);
    RefCells *objects[] = {first, first == second ? NULL : second};
    invoke(files, objects, width ? call.ret : NULL, width, 1);
}
