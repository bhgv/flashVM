/* Copyright (c) 2026 bhgv
 * EN: Guest-owned immutable tree capture/composition and ordinary indexed mount/bind.
 * RU: Guest-owned immutable tree capture/composition и indexed mount/bind.
 */
#include "cap_internal.h"
#include "view_internal.h"
#include "native_wait.h"
#include "path_cancel.h"
#include "cancel.h"
#include "bind_wait.h"
#include <string.h>
#include <limits.h>


/* EN/RU: Capture one explicit effective candidate, preserving capability-relative routing.
 * Захватить один explicit effective candidate, сохраняя capability-relative routing. */
static DVM_Provider *capture(DVM_ResourcePlan *plan, int64_t index, unsigned access)
{
    size_t count = 0, position;
    DVM_ResourcePlan *entry;
    for (entry = plan; entry; entry = entry->layers)
    {
        count++;
    }
    if (!dvm_stack_position(index, count, 0, &position))
    {
        return NULL;
    }
    entry = plan;
    for (size_t ordinal = 0; ordinal < position; ordinal++)
    {
        entry = entry->layers;
    }
    FVM_FileInfo info;
    if (dvm_cancel_requested(dvm_cancel_current()) || dvm_resource_raw_info(entry, &info) != FVM_OK)
    {
        return NULL;
    }
    DVM_Provider *source = entry->provider;
    const char *relative = entry->provider ? entry->relative : ".";
    if (source && !dvm_provider_bindable_at(source, relative))
    {
        source = dvm_provider_capture(source, relative);
        relative = ".";
    }
    else if (source ? !dvm_provider_retain(source) : !(source = dvm_export_native(entry, info.directory)))
    {
        return NULL;
    }
    if (!source || dvm_cancel_requested(dvm_cancel_current()))
    {
        dvm_provider_release(source);
        return NULL;
    }
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view || !dvm_view_map(view, "/", source, relative, access,
                               info.directory ? DVM_VIEW_TREE : 0))
    {
        dvm_view_discard(view);
        dvm_provider_release(source);
        return NULL;
    }
    dvm_provider_release(source);
    return dvm_view_freeze(view);
}


/* EN/RU: Copy path under admission, then native capture; no source path leaks to guest.
 * Копировать path под admission, затем native capture; host source path не выдаётся. */
void Vfsexport_capture(void *frame)
{
    F_Vfsexport_capture call;
    char path[DVM_NS_PATH_LIMIT];
    DVM_CapState *state;
    DVM_ResourcePlan plan;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_capture, ret), sizeof(*call.ret));
    if (call.rights < 1 || call.rights > FVM_ACCESS_MASK || !dvm_cap_text(call.path, path, sizeof(path)))
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (waserror())
    {
        dvm_cap_abandon(value);
        nexterror();
    }
    if (state && dvm_resource_plan(dvm_namespace_current_group(), path, 0, &plan))
    {
        DVM_VfsWait *operation = NULL;
        int released = dvm_capture_cancellable(&plan) ? dvm_vfs_path_enter(&operation) : dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            state->root = capture(&plan, call.index, call.rights);
            int cancelled = dvm_vfs_wait_end(operation);
            dvm_runtime_gc_native_enter(released);
            if (!state->root && cancelled)
            {
                kwerrstr("native VFS capture cancelled");
            }
        }
        dvm_resource_close(&plan);
    }
    int success = state && state->root;
    poperror();
    if (!success)
    {
        dvm_cap_abandon(value);
        value = H;
    }
    dvm_cap_publish(value, call.ret);
}


/* [GC:] EN: Capture a child of a borrowed capability without mounting it.
 * The rooted result owns the captured provider and preserves its access.
 * RU: Захватить узел заимствованного полномочия без маунта. Укоренённый
 * результат владеет захваченным провайдером и сохраняет его права. */
void Vfsexport_subtree(void *frame)
{
    F_Vfsexport_subtree call;
    char path[DVM_NS_PATH_LIMIT];
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_subtree, ret), sizeof(*call.ret));
    DVM_CapState *source = dvm_cap_get(call.root, DVM_CAP_ROOT);
    if (!source || !source->root || !dvm_cap_text(call.relative, path, sizeof(path)) || !dvm_view_path(path, 0))
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    DVM_CapState *state;
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (waserror())
    {
        dvm_cap_abandon(value);
        nexterror();
    }
    int valid = 0;
    if (state)
    {
        DVM_VfsWait *wait;
        int released = dvm_vfs_path_enter(&wait);
        if (released >= 0)
        {
            state->root = dvm_provider_capture(source->root, path);
            FVM_FileInfo info;
            int found = state->root && dvm_provider_info(state->root, ".", &info) == FVM_OK;
            int cancelled = dvm_vfs_wait_end(wait);
            valid = found && !cancelled;
            dvm_runtime_gc_native_enter(released);
        }
    }
    poperror();
    if (!valid)
    {
        dvm_cap_abandon(value);
        value = H;
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Overlay one source or hide rule onto a retained immutable base tree.
 * Наложить source/hide rule поверх immutable base; failed staging changes no tree. */
void Vfsexport_overlay(void *frame)
{
    F_Vfsexport_overlay call;
    char path[DVM_NS_PATH_LIMIT];
    DVM_CapState *state;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_overlay, ret), sizeof(*call.ret));
    DVM_CapState *base = dvm_cap_get(call.base, DVM_CAP_ROOT);
    DVM_CapState *source = dvm_cap_get(call.source, DVM_CAP_ROOT);
    if (call.rights < 0 || call.rights > FVM_ACCESS_MASK || call.flags < 0 || call.flags > DVM_VIEW_TREE ||
        (source && !call.rights) || !dvm_cap_text(call.path, path, sizeof(path)))
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (waserror())
    {
        dvm_cap_abandon(value);
        nexterror();
    }
    if (state)
    {
        int released = dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            DVM_View *view = dvm_view_create(NULL, NULL, NULL);
            if (view && (!base || dvm_view_map(view, "/", base->root, ".", FVM_ACCESS_MASK, DVM_VIEW_TREE)) &&
                dvm_view_map(view, path, source ? source->root : NULL, ".", call.rights, call.flags))
            {
                state->root = dvm_view_freeze(view);
            }
            else
            {
                dvm_view_discard(view);
            }
            dvm_runtime_gc_native_enter(released);
        }
    }
    int success = state && state->root;
    poperror();
    if (!success)
    {
        dvm_cap_abandon(value);
        value = H;
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Observe actual granted node kind/rights before selecting a mount target.
 * Наблюдать actual granted node kind/rights до выбора mount target. */
void Vfsexport_info(void *frame)
{
    F_Vfsexport_info call;
    WORD result[3] = {-1, 0, 0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_info, ret), sizeof(*call.ret));
    DVM_CapState *state = dvm_cap_get(call.root, DVM_CAP_ROOT);
    if (state)
    {
        DVM_VfsWait *operation = NULL;
        int released = dvm_provider_native_flags(state->root) & DVM_PROVIDER_CANCEL_PATH ?
            dvm_vfs_path_enter(&operation) : dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            FVM_FileInfo metadata;
            FVM_Result status = dvm_provider_info(state->root, ".", &metadata);
            if (status == FVM_OK)
            {
                result[0] = 0;
                result[1] = metadata.mode & 0777;
                result[2] = metadata.directory;
            }
            int cancelled = dvm_vfs_wait_end(operation);
            dvm_runtime_gc_native_enter(released);
            if (status != FVM_OK && cancelled)
            {
                kwerrstr("native VFS root info cancelled");
            }
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Mount a captured tree only in caller's namespace, native effects execute once.
 * Mount captured tree только в своём namespace; native effects выполняются один раз. */
void Vfsexport_mount(void *frame)
{
    F_Vfsexport_mount call;
    char path[DVM_NS_PATH_LIMIT];
    WORD result = -1;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_mount, ret), sizeof(*call.ret));
    DVM_CapState *state = dvm_cap_get(call.root, DVM_CAP_ROOT);
    if (state && call.flags >= 0 && call.flags <= UINT_MAX && dvm_cap_text(call.target, path, sizeof(path)))
    {
        DVM_ResourcePlan target, source = {0};
        DVM_Namespace *group = dvm_namespace_current_group();
        if (dvm_resource_plan(group, path, 0, &target))
        {
            source.group = group;
            source.names.version = target.names.version;
            source.provider = state->root;
            strcpy(source.names.path, "/");
            strcpy(source.names.native, "/export-capability");
            strcpy(source.relative, ".");
            int cancelled;
            int outcome = dvm_bind_wait(&source, &target, call.flags, call.index, &cancelled);
            result = outcome == 1 ? 0 : outcome == 0 && !cancelled ? -2 : -1;
            dvm_resource_close(&target);
            if (outcome != 1 && cancelled)
            {
                kwerrstr("native capability mount cancelled");
            }
        }
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}
