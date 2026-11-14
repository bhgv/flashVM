/* Copyright (c) 2026 bhgv
 * EN: Standard Sys namespace subset on native path plans, with explicit refusals.
 * RU: Подмножество стандартного Sys namespace на native-планах; явные отказы.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "kernel.h"
#include "runqueue.h"
#include "sys_capture.h"
#include "metadata_wait.h"
#include "guest.h"
#include <string.h>

#define DVM_NS_RETRIES 8                      /* Bounded retries when another owner changes the shared namespace. */
extern char *dvm_runtime_file_path(String *); /* Borrowed validated guest UTF-8 path. */


/* EN/RU: Borrow current permanent owner's group; call only while ACTIVE.
 * Заимствовать группу текущего постоянного owner; вызывать только ACTIVE. */
static DVM_Namespace *current_group(void)
{
    Prog *process = currun();
    return process ? ((DVM_RuntimeProcess *)process)->namespace_group : NULL;
}


/* EN/RU: See guest.h; no namespace pointer escapes or survives native I/O.
 * См. guest.h; указатель namespace не передаётся в native I/O. */
int dvm_namespace_host_path(const char *path, char *output)
{
    DVM_Namespace *group = current_group();
    DVM_NamespacePlan plan;
    if (!group)
    {
        if (!path || strlen(path) >= DVM_NS_PATH_LIMIT)
        {
            kwerrstr("invalid or oversized host path");
            return 0;
        }
        memcpy(output, path, strlen(path) + 1);
        return 1;
    }
    if (!dvm_namespace_plan(group, path, &plan))
    {
        kwerrstr("invalid or oversized namespace path");
        return 0;
    }
    memcpy(output, plan.native, strlen(plan.native) + 1);
    return 1;
}


/* EN: Copy borrowed rooted String into private native path. Invalid content
 * returns zero with guest error; malformed/nil values retain ordinary VM errors.
 * RU: Скопировать укоренённый String в native path. Неверное содержимое — ноль
 * с guest error; malformed/nil сохраняют обычные VM errors. */
static int copy_path(String *name, char *output)
{
    char *path = dvm_runtime_file_path(name);
    if (!path)
    {
        return 0;
    }
    if (!*path || strlen(path) >= DVM_NS_PATH_LIMIT)
    {
        kwerrstr("invalid or oversized namespace path");
        return 0;
    }
    memcpy(output, path, strlen(path) + 1);
    return 1;
}


/* [GC:] EN: Guard publication of one staged WORD into a captured shared result.
 * Retry only publication; never repeat namespace effects. Caller retains roots.
 * RU: Публиковать staged WORD в shared result под guards. Повторять только
 * publication, не namespace effects. Caller удерживает корни. */
static void publish(WORD *destination, int result)
{
    WORD value = result;
    dvm_sys_scalar_result(&value, destination, sizeof(value));
}


/* EN: Detach namespace only: FORKNS copies, NEWNS roots at the resolved cwd.
 * Other flags/movefd return -1 without changes. Capture frame before allocation.
 * RU: Отделить namespace: FORKNS копирует, NEWNS берёт resolved cwd как root.
 * Другие flags/movefd возвращают -1 без изменений. Захватить frame до allocation. */
void Sys_pctl(void *frame)
{
    F_Sys_pctl call;
    Prog *process = currun();
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    DVM_Namespace *replacement = NULL;
    DVM_NamespacePlan cwd;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_pctl, ret), sizeof(*call.ret));
    if ((call.flags != 0 && call.flags != Sys_FORKNS && call.flags != Sys_NEWNS) || call.movefd != H)
    {
        kwerrstr("pctl supports namespace-only FORKNS or NEWNS with nil movefd");
        publish(call.ret, -1);
        return;
    }
    if (!call.flags)
    {
        publish(call.ret, process->pid);
        return;
    }
    if (call.flags == Sys_FORKNS)
    {
        replacement = dvm_namespace_copy(owner->namespace_group);
    }
    else if (dvm_namespace_plan(owner->namespace_group, ".", &cwd))
    {
        replacement = dvm_namespace_create(cwd.native, "/");
    }
    if (!replacement)
    {
        kwerrstr("cannot allocate namespace group");
        publish(call.ret, -1);
        return;
    }
    DVM_Namespace *previous = owner->namespace_group;
    owner->namespace_group = replacement;
    dvm_namespace_release(previous);
    publish(call.ret, process->pid);
}


/* EN: Validate directory outside namespace lock; commit only its observed
 * revision. Concurrent changes retry at most eight times; host cwd never changes.
 * RU: Проверить каталог вне namespace mutex; применить только наблюдавшуюся
 * версию. До восьми повторов при изменениях; host cwd не меняется. */
void Sys_chdir(void *frame)
{
    F_Sys_chdir call;
    DVM_NamespacePlan plan;
    FVM_FileInfo info;
    char path[DVM_NS_PATH_LIMIT];
    int result = -1, attempt;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_chdir, ret), sizeof(*call.ret));
    if (copy_path(call.path, path))
    {
        for (attempt = 0; attempt < DVM_NS_RETRIES; attempt++)
        {
            if (!dvm_namespace_plan(current_group(), path, &plan))
            {
                kwerrstr("invalid namespace directory path");
                break;
            }
            if (dvm_metadata_native_path_info(plan.native, &info) != FVM_OK)
            {
                break;
            }
            if (!info.directory)
            {
                kwerrstr("chdir requires a directory");
                break;
            }
            int committed = dvm_namespace_chdir(current_group(), &plan);
            if (committed)
            {
                result = committed == 1 ? 0 : -1;
                if (committed < 0)
                {
                    kwerrstr("namespace revision exhausted");
                }
                break;
            }
        }
        if (attempt == DVM_NS_RETRIES)
        {
            kwerrstr("namespace changed during chdir");
        }
    }
    publish(call.ret, result);
}


/* EN: MREPL binds existing like-kind host resources; frozen source plans avoid
 * alias recursion. Unsupported union/create/cache flags return -1 atomically.
 * RU: MREPL связывает существующие однотипные host-ресурсы; frozen source
 * исключает alias-рекурсию. Union/create/cache flags дают -1 без изменений. */
void Sys_bind(void *frame)
{
    F_Sys_bind call;
    DVM_NamespacePlan source, target;
    FVM_FileInfo source_info, target_info;
    char from[DVM_NS_PATH_LIMIT], on[DVM_NS_PATH_LIMIT];
    int result = -1, attempt;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_bind, ret), sizeof(*call.ret));
    if (call.flags != Sys_MREPL)
    {
        kwerrstr("bind supports MREPL only");
    }
    else if (copy_path(call.s, from) && copy_path(call.on, on))
    {
        for (attempt = 0; attempt < DVM_NS_RETRIES; attempt++)
        {
            DVM_Namespace *group = current_group();
            if (!dvm_namespace_plan(group, from, &source) || !dvm_namespace_plan(group, on, &target))
            {
                kwerrstr("invalid namespace bind path");
                break;
            }
            if (source.version != target.version)
            {
                continue;
            }
            if (dvm_metadata_native_path_info(source.native, &source_info) != FVM_OK ||
                dvm_metadata_native_path_info(target.native, &target_info) != FVM_OK)
            {
                break;
            }
            if (source_info.directory != target_info.directory)
            {
                kwerrstr("bind source and target kinds differ");
                break;
            }
            int committed = dvm_namespace_bind(group, &source, &target);
            if (committed)
            {
                result = committed == 1 ? 0 : -1;
                if (committed < 0)
                {
                    kwerrstr("cannot allocate namespace binding");
                }
                break;
            }
        }
        if (attempt == DVM_NS_RETRIES)
        {
            kwerrstr("namespace changed during bind");
        }
    }
    publish(call.ret, result);
}


/* EN: Remove one MREPL binding; nil/empty source removes the exact target.
 * No host I/O or descriptor invalidation. Version conflicts retry boundedly.
 * RU: Удалить MREPL-привязку; nil/empty source удаляет точный target. Без
 * host I/O/инвалидации FD. Конфликты версий повторяются ограниченно. */
void Sys_unmount(void *frame)
{
    F_Sys_unmount call;
    DVM_NamespacePlan source, target;
    char from[DVM_NS_PATH_LIMIT], on[DVM_NS_PATH_LIMIT];
    int result = -1, attempt, filtered;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_unmount, ret), sizeof(*call.ret));
    const char *filter = call.s1 == H || call.s1 == nil ? "" : dvm_runtime_file_path(call.s1);
    if (!filter)
    {
        publish(call.ret, -1);
        return;
    }
    filtered = *filter != 0;
    if ((!filtered || copy_path(call.s1, from)) && copy_path(call.s2, on))
    {
        for (attempt = 0; attempt < DVM_NS_RETRIES; attempt++)
        {
            DVM_Namespace *group = current_group();
            if (!dvm_namespace_plan(group, on, &target) || (filtered && !dvm_namespace_plan(group, from, &source)))
            {
                kwerrstr("invalid namespace unmount path");
                break;
            }
            int committed = dvm_namespace_unmount(group, &target, filtered ? &source : NULL);
            if (committed)
            {
                result = committed == 1 ? 0 : -1;
                if (committed < 0)
                {
                    kwerrstr("namespace binding not found");
                }
                break;
            }
        }
        if (attempt == DVM_NS_RETRIES)
        {
            kwerrstr("namespace changed during unmount");
        }
    }
    publish(call.ret, result);
}
