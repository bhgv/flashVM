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
#include "runtime_gc.h"
#include "metadata_wait.h"
#include "guest.h"
#include "boot.h"
#include "resource_wait.h"
#include "sysevents.h"
#include "stack.h"
#include "bind_wait.h"
#include <string.h>

#define DVM_NS_RETRIES 8                      /* Bounded retries when another owner changes the shared namespace. */
extern char *dvm_runtime_file_path(String *); /* Borrowed validated guest UTF-8 path. */


/* EN/RU: See guest.h; ACTIVE guest or startup controller only.
 * См. guest.h; только ACTIVE guest либо startup controller. */
DVM_Namespace *dvm_namespace_current_group(void)
{
    Prog *process = currun();
    return process ? ((DVM_RuntimeProcess *)process)->namespace_group : dvm_namespace_boot_group();
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


/* EN: Detach namespace with coherent retained directory identities. Unsupported
 * flags/movefd leave the group unchanged; frame is captured before allocation.
 * RU: Отделить namespace с retained identities. Неподдержанные flags/movefd
 * не меняют группу; frame захватывается до allocation. */
void Sys_pctl(void *frame)
{
    F_Sys_pctl call;
    Prog *process = currun();
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    DVM_Namespace *replacement = NULL;
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
    else
    {
        DVM_ResourcePlan cwd;
        char component[DVM_NS_PATH_LIMIT];
        if (dvm_resource_plan(owner->namespace_group, ".", 0, &cwd))
        {
            if (dvm_resource_anchor_wait(&cwd, 1, component))
            {
                replacement = dvm_namespace_rebase(&cwd);
            }
            dvm_resource_close(&cwd);
        }
    }
    if (!replacement)
    {
        kwerrstr("cannot allocate or acquire namespace group");
        publish(call.ret, -1);
        return;
    }
    if (!dvm_sysevents_group(owner->system_events, replacement))
    {
        dvm_namespace_release(replacement);
        kwerrstr("cannot replace system-event namespace scope");
        publish(call.ret, -1);
        return;
    }
    DVM_Namespace *previous = owner->namespace_group;
    owner->namespace_group = replacement;
    dvm_namespace_release(previous);
    publish(call.ret, process->pid);
}


/* EN: Open cwd resource outside namespace mutex; commit only matching revision.
 * RU: Открыть cwd resource вне namespace mutex; применить только совпавшую версию.
 * Captured frame/path remain rooted / Захваченные frame/path укоренены. */
void Sys_chdir(void *frame)
{
    F_Sys_chdir call;
    char path[DVM_NS_PATH_LIMIT], component[DVM_NS_PATH_LIMIT];
    int result = -1, attempt;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_chdir, ret), sizeof(*call.ret));
    if (copy_path(call.path, path))
    {
        for (attempt = 0; attempt < DVM_NS_RETRIES; attempt++)
        {
            DVM_ResourcePlan plan;
            if (!dvm_resource_plan(dvm_namespace_current_group(), path, 0, &plan))
            {
                kwerrstr("invalid namespace directory path");
                break;
            }
            FVM_FileInfo metadata;
            int acquired = !plan.union_root && dvm_metadata_selected(&plan, &metadata) == FVM_OK && metadata.directory &&
                dvm_resource_anchor_wait(&plan, 1, component);
            if (plan.union_root)
            {
                kwerrstr("chdir into union root needs retained Chan cwd");
            }
            int committed = acquired ? dvm_namespace_chdir_resource(dvm_namespace_current_group(), &plan) : -1;
            dvm_resource_close(&plan);
            if (committed)
            {
                result = committed == 1 ? 0 : -1;
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


/* EN: Bind directory identity or a held file-parent/basename at exact target.
 * Native validation/acquisition runs unlocked; stale revisions retry boundedly.
 * RU: Связать identity каталога либо held parent/basename файла с target.
 * Native validation/acquisition вне mutex; stale versions повторяются ограниченно. */
void Sys_bind(void *frame)
{
    F_Sys_bind call;
    char from[DVM_NS_PATH_LIMIT], on[DVM_NS_PATH_LIMIT], component[DVM_NS_PATH_LIMIT], target_component[DVM_NS_PATH_LIMIT];
    int result = -1, attempt;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_bind, ret), sizeof(*call.ret));
    if ((call.flags & ~(Sys_MBEFORE | Sys_MAFTER | Sys_MCREATE | DVM_NS_AUTORWX)) ||
        (call.flags & (Sys_MBEFORE | Sys_MAFTER)) == (Sys_MBEFORE | Sys_MAFTER))
    {
        kwerrstr("invalid bind order/create flags");
    }
    else if (copy_path(call.s, from) && copy_path(call.on, on))
    {
        for (attempt = 0; attempt < DVM_NS_RETRIES; attempt++)
        {
            DVM_Namespace *group = dvm_namespace_current_group();
            DVM_ResourcePlan source, target;
            FVM_FileInfo a, b;
            if (!dvm_resource_plan(group, from, 0, &source))
            {
                kwerrstr("invalid namespace bind source");
                break;
            }
            if (!dvm_resource_plan(group, on, 0, &target))
            {
                dvm_resource_close(&source);
                kwerrstr("invalid namespace bind target");
                break;
            }
            int committed = 0;
            if (source.names.version == target.names.version)
            {
                committed = -1;
                FVM_Result source_status = FVM_ERROR;
                if (dvm_capture_cancellable(&source) && dvm_capture_cancellable(&target))
                {
                    int cancelled;
                    committed = dvm_bind_wait(&source, &target, call.flags, 0, &cancelled);
                    if (committed != 1 && cancelled)
                    {
                        committed = -1;
                        kwerrstr("native bind cancelled");
                    }
                }
                else if ((source_status = dvm_metadata_selected(&source, &a)) == FVM_OK && (!a.directory ||
                    dvm_provider_capturable(source.provider) || dvm_provider_capturable(target.provider)))
                {
                    int released = dvm_runtime_gc_native_try_leave();
                    if (released >= 0)
                    {
                        committed = dvm_stack_bind(&source, &target, call.flags, 0);
                        dvm_runtime_gc_native_enter(released);
                    }
                    if (committed < 0)
                    {
                        kwerrstr("captured bind target/stack refused");
                    }
                }
                else if (source_status == FVM_OK && dvm_metadata_selected(&target, &b) == FVM_OK)
                {
                    if (a.directory != b.directory)
                    {
                        kwerrstr("bind source and target kinds differ");
                    }
                    else if ((call.flags & (Sys_MBEFORE | Sys_MAFTER)) && !a.directory)
                    {
                        kwerrstr("union bind requires directories");
                    }
                    else if ((!source.provider || dvm_provider_bindable(source.provider)) &&
                             (source.provider || dvm_resource_anchor_wait(&source, a.directory, component)) &&
                             (!(call.flags & (Sys_MBEFORE | Sys_MAFTER)) || target.provider ||
                              dvm_resource_anchor_wait(&target, 1, target_component)))
                    {
                        committed = dvm_namespace_bind_layers(group, &source, &target, call.flags, a.directory, component);
                        if (committed < 0)
                        {
                            kwerrstr("bind union/identity contract refused");
                        }
                    }
                    else
                    {
                        kwerrstr("provider lacks stable bind identity or directory acquisition failed");
                    }
                }
            }
            dvm_resource_close(&target);
            dvm_resource_close(&source);
            if (committed)
            {
                result = committed == 1 ? 0 : -1;
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


/* EN/RU: Compare complete host identity, not a stale display spelling.
 * Сравнить полную host identity, не устаревший display path. Pure native values. */
static int same_resource(const FVM_FileInfo *a, const FVM_FileInfo *b)
{
    return a->identity.high == b->identity.high && a->identity.low == b->identity.low &&
           a->device.high == b->device.high && a->device.low == b->device.low && a->directory == b->directory;
}


/* EN: Unmount exact target; optional source filtering compares opened identities.
 * Namespace changes retry, existing plans/FDs retain their owned resources.
 * RU: Удалить точный target; source filter сравнивает opened identities.
 * Изменения namespace повторяются, живые plans/FDs держат свои resources. */
void Sys_unmount(void *frame)
{
    F_Sys_unmount call;
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
            DVM_Namespace *group = dvm_namespace_current_group();
            DVM_ResourcePlan target, source;
            FVM_FileInfo a, b;
            if (!dvm_resource_plan(group, on, 0, &target))
            {
                kwerrstr("invalid namespace unmount target");
                break;
            }
            int source_live = filtered && dvm_resource_plan(group, from, 0, &source);
            int committed = 0;
            if (filtered && !source_live)
            {
                dvm_resource_close(&target);
                kwerrstr("invalid namespace unmount source");
                break;
            }
            if (!filtered || (source_live && source.names.version == target.names.version))
            {
                committed = -1;
                if (!filtered)
                {
                    committed = dvm_namespace_unmount_layer(group, &target.names, 0);
                }
                else
                {
                    DVM_ResourcePlan *choice;
                    int source_ok = source.provider || dvm_metadata_selected(&source, &a) == FVM_OK;
                    for (choice = &target; source_ok && choice; choice = choice->layers)
                    {
                        /* Each filter comparison belongs to this branch, never a lower fallback. */
                        /* Каждое filter-сравнение относится к этой ветви, без lower fallback. */
                        choice->selected = 1;
                        int match = source.provider || choice->provider ?
                            source.provider == choice->provider && !strcmp(source.relative, choice->relative) :
                            dvm_metadata_selected(choice, &b) == FVM_OK && same_resource(&a, &b);
                        if (match)
                        {
                            committed = dvm_namespace_unmount_layer(group, &target.names, choice->layer_id);
                            break;
                        }
                    }
                    if (committed < 0)
                    {
                        kwerrstr("unmount source identity mismatch");
                    }
                }
            }
            if (source_live)
            {
                dvm_resource_close(&source);
            }
            dvm_resource_close(&target);
            if (committed)
            {
                result = committed == 1 ? 0 : -1;
                if (committed < 0 && !filtered)
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
