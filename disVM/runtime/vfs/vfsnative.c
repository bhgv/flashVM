/* Copyright (c) 2026 bhgv
 * EN: Captured scalar VFS bridge; native queries release admission, effects publish once.
 * RU: Captured scalar VFS bridge; native queries вне admission, effects публикуются один раз.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "kernel.h"
#include "sys_capture.h"
#include "runtime_gc.h"
#include "guest.h"
#include "internal.h"
#include "stack.h"
#include "bind_wait.h"
#include "cancel.h"
#include "native_wait.h"
#include "launch/route.h"
#include "runqueue.h"
#include "vfsnative_decl.h"
#include "vfsnative_table.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
extern char *dvm_runtime_file_path(String *name); /* Borrowed validated UTF-8, caller frame rooted. */


/* EN/RU: Copy rooted guest path before native release; no pointer survives in native work.
 * Копировать rooted path до native release; zero on invalid path, no effects. */
static int path(String *name, char *output)
{
    char *value = dvm_runtime_file_path(name);
    if (!value || !*value || strlen(value) >= DVM_NS_PATH_LIMIT)
    {
        kwerrstr("invalid Vfsctl path");
        return 0;
    }
    strcpy(output, value);
    return 1;
}


/* EN/RU: Borrow an effective candidate at signed index in one retained plan.
 * Заимствовать effective candidate по signed index, return NULL/out-of-range. */
static DVM_ResourcePlan *choice(DVM_ResourcePlan *plan, int64_t index, size_t *count, size_t *position)
{
    DVM_ResourcePlan *entry;
    *count = 0;
    for (entry = plan; entry; entry = entry->layers)
    {
        (*count)++;
    }
    if (!dvm_stack_position(index, *count, 0, position))
    {
        return NULL;
    }
    entry = plan;
    for (size_t ordinal = 0; ordinal < *position; ordinal++)
    {
        entry = entry->layers;
    }
    return entry;
}


/* EN/RU: Snapshot effective access, encoding allowed bits low and known bits at <<8.
 * Query errors deny; UNSUPPORTED leaves unknown. Native-only, no namespace locks.
 * Снять effective access: allowed low bits, known<<8; UNSUPPORTED не выдумывает права. */
static unsigned rights(DVM_ResourcePlan *entry)
{
    unsigned result = 0;
    for (unsigned bit = 1; bit <= FVM_ACCESS_READ; bit <<= 1)
    {
        if (dvm_cancel_requested(dvm_cancel_current()))
        {
            break;
        }
        FVM_Result status = entry->provider ? dvm_provider_access(entry->provider, entry->relative, bit) :
            entry->anchor ? fvm_port_anchor_access_at(dvm_anchor_port(entry->anchor), entry->relative, bit) : FVM_UNSUPPORTED;
        if (dvm_cancel_requested(dvm_cancel_current()))
        {
            break;
        }
        if (status != FVM_UNSUPPORTED)
        {
            result |= bit << 8;
            if (status == FVM_OK)
            {
                result |= bit;
            }
        }
    }
    return result;
}


/* EN/RU: Capture copied metadata for one branch; stale config zeros the payload.
 * Снять metadata одной ветви; stale config обнуляет payload. No live pointer results. */
void Vfsnative_layer(void *frame)
{
    F_Vfsnative_layer call;
    LONG result[12] = {-1}; /* Pure scalar tuple, exact generated NC64 extent. */
    char name[DVM_NS_PATH_LIMIT];
    DVM_ResourcePlan plan, *entry;
    size_t count, position;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_layer, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.revision >= -1 && path(call.path, name) && dvm_resource_plan(dvm_namespace_current_group(), name, 0, &plan))
    {
        if (plan.names.version > LLONG_MAX || (call.revision >= 0 && (uint64_t)call.revision != plan.names.version))
        {
            result[0] = -2;
        }
        else
        {
            int cancelled = 0;
            entry = choice(&plan, call.index, &count, &position);
            result[0] = entry ? -1 : 0;
            if (entry && count <= INT_MAX && plan.head_id <= LLONG_MAX && entry->layer_id <= LLONG_MAX)
            {
                FVM_FileInfo info;
                DVM_VfsWait *operation = NULL;
                int released = dvm_provider_native_flags(entry->provider) & DVM_PROVIDER_CANCEL_PATH ?
                    dvm_vfs_path_enter(&operation) : dvm_runtime_gc_native_try_leave();
                if (released >= 0)
                {
                    FVM_Result status = dvm_resource_raw_info(entry, &info);
                    unsigned access = rights(entry);
                    cancelled = dvm_vfs_wait_end(operation);
                    dvm_runtime_gc_native_enter(released);
                    result[0] = 1;
                    result[1] = plan.names.version;
                    result[2] = plan.head_id;
                    result[3] = entry->layer_id;
                    result[4] = count;
                    result[5] = position;
                    result[6] = entry->mount_flags;
                    result[7] = entry->provider != NULL;
                    result[8] = status;
                    result[9] = status == FVM_OK ? info.mode : 0;
                    result[10] = status == FVM_OK ? info.directory : 0;
                    result[11] = access;
                }
                else
                {
                    cancelled = released == -2;
                    result[0] = -1;
                }
            }
            if (cancelled || !dvm_stack_current(plan.group, plan.names.version))
            {
                memset(result, 0, sizeof(result));
                result[0] = cancelled ? -1 : -2;
                if (cancelled)
                {
                    kwerrstr("native layer inspection cancelled");
                }
            }
        }
        dvm_resource_close(&plan);
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Project one bounded text byte with config revision fencing.
 * Читать один bounded text byte с config revision fence; no host source path output. */
void Vfsnative_text(void *frame)
{
    F_Vfsnative_text call;
    LONG result[2] = {-1, 0};
    char name[DVM_NS_PATH_LIMIT], text[DVM_NS_PATH_LIMIT];
    DVM_ResourcePlan plan, *entry;
    size_t count, position;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_text, ret), sizeof(*call.ret));
    if (call.revision >= 0 && call.part >= 0 && call.part <= 6 && call.offset >= 0 &&
        path(call.path, name) && dvm_resource_plan(dvm_namespace_current_group(), name, 0, &plan))
    {
        result[0] = -2;
        if ((uint64_t)call.revision == plan.names.version)
        {
            entry = choice(&plan, call.index, &count, &position);
            if (entry)
            {
                if (call.part >= 2)
                {
                    const char *parts[] = {entry->origin.role == DVM_ORIGIN_TERMINAL ? "1" :
                        entry->origin.role == DVM_ORIGIN_PROVIDER ? "2" : "", entry->origin.system,
                        entry->origin.export_id, entry->origin.session, entry->origin.nickname};
                    strcpy(text, parts[call.part-2]);
                }
                else if (call.part)
                {
                    strcpy(text, entry->relative);
                }
                else
                {
                    strcpy(text, "/");
                    fvm_port_sync_lock(plan.group->lock);
                    for (DVM_NamespaceBinding *binding = plan.group->bindings; binding; binding = binding->next)
                    {
                        if (binding->identity == entry->layer_id)
                        {
                            strcpy(text, binding->on);
                            break;
                        }
                    }
                    fvm_port_sync_unlock(plan.group->lock);
                }
                size_t length = strlen(text);
                result[0] = (uint64_t)call.offset < length ? 1 : (uint64_t)call.offset == length ? 0 : -1;
                if (result[0] == 1)
                {
                    result[1] = (unsigned char)text[call.offset];
                }
            }
            else
            {
                result[0] = -1;
            }
            if (!dvm_stack_current(plan.group, plan.names.version))
            {
                result[0] = -2;
                result[1] = 0;
            }
        }
        dvm_resource_close(&plan);
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Copy provider-local journal state/capability without source I/O.
 * Скопировать provider-local journal state/capability без source I/O. */
void Vfsnative_state(void *frame)
{
    F_Vfsnative_state call;
    LONG result[4] = {-1, 0, 0, 0};
    char name[DVM_NS_PATH_LIMIT];
    DVM_ResourcePlan plan, *entry;
    size_t count, position;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_state, ret), sizeof(*call.ret));
    if (call.revision >= 0 && path(call.path, name) && dvm_resource_plan(dvm_namespace_current_group(), name, 0, &plan))
    {
        result[0] = -2;
        entry = choice(&plan, call.index, &count, &position);
        if ((uint64_t)call.revision == plan.names.version && entry)
        {
            result[0] = 1;
            if (entry->provider)
            {
                DVM_ChangeCursor empty = {0};
                DVM_ChangeBatch batch;
                if (dvm_changes_pending(dvm_provider_changes(entry->provider), &empty, DVM_CHANGE_ALL, &batch) != 1 ||
                    batch.event.session > LLONG_MAX || batch.event.generation > LLONG_MAX)
                {
                    result[0] = -1;
                }
                else
                {
                    result[1] = batch.event.session;
                    result[2] = batch.event.generation;
                    result[3] = (dvm_provider_bindable_at(entry->provider, entry->relative) ? DVM_PROVIDER_STABLE_BIND : 0) |
                        (dvm_provider_capturable(entry->provider) ? 2 : 0);
                }
            }
            if (!dvm_stack_current(plan.group, plan.names.version))
            {
                memset(result, 0, sizeof(result));
                result[0] = -2;
            }
        }
        dvm_resource_close(&plan);
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Stage both paths, run native checked bind once per captured revision.
 * Подготовить оба path; native checked bind по captured revision, no effect replay. */
void Vfsnative_bind(void *frame)
{
    F_Vfsnative_bind call;
    WORD result = -1;
    char from[DVM_NS_PATH_LIMIT], on[DVM_NS_PATH_LIMIT];
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_bind, ret), sizeof(*call.ret));
    if (call.flags >= 0 && call.flags <= UINT_MAX && path(call.source, from) && path(call.target, on))
    {
        DVM_ResourcePlan source, target;
        if (dvm_resource_plan(dvm_namespace_current_group(), from, 0, &source))
        {
            if (dvm_resource_plan(source.group, on, 0, &target))
            {
                int cancelled;
                int outcome = dvm_bind_wait(&source, &target, (unsigned)call.flags, call.index, &cancelled);
                result = outcome == 1 ? 0 : outcome == 0 && !cancelled ? -2 : -1;
                if (outcome != 1 && cancelled)
                {
                    kwerrstr("native indexed bind cancelled");
                }
                dvm_resource_close(&target);
            }
            dvm_resource_close(&source);
        }
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Move one observed layer; caller can retry stale after a fresh inspection.
 * Переставить observed layer; stale требует нового inspection, never automatic replay. */
void Vfsnative_move(void *frame)
{
    F_Vfsnative_move call;
    WORD result = -1;
    char name[DVM_NS_PATH_LIMIT];
    DVM_ResourcePlan target;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_move, ret), sizeof(*call.ret));
    if (call.revision >= 0 && call.id >= 0 && path(call.path, name) &&
        dvm_resource_plan(dvm_namespace_current_group(), name, 0, &target))
    {
        int cancelled;
        int outcome = dvm_move_wait(&target, call.revision, call.id, call.index, &cancelled);
        result = outcome == 1 ? 0 : outcome == 0 && !cancelled ? -2 : -1;
        if (outcome != 1 && cancelled)
        {
            kwerrstr("native layer move cancelled");
        }
        dvm_resource_close(&target);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Resolve the copied path and submit native bytes outside managed admission.
 * Разрешить скопированный путь и отправить нативные байты вне допуска VM. */
static WORD run_path(const char *name, const void *bytes, size_t size)
{
    WORD result = DVM_LAUNCH_NOT_SENT;
    Prog *process = currun();
    DVM_Namespace *group = dvm_namespace_current_group();
    if (process && group && dvm_namespace_retain(group))
    {
        unsigned role = ((DVM_RuntimeProcess *)process)->module_local_role;
        DVM_VfsWait *wait;
        int released = dvm_vfs_path_enter(&wait);
        if (released >= 0)
        {
            DVM_LaunchAddress *address = dvm_launch_route_resolve(group, name, role);
            result = dvm_launch_address_send(address, bytes, size);
            dvm_launch_address_free(address);
            dvm_vfs_wait_end(wait);
            dvm_runtime_gc_native_enter(released);
        }
        dvm_namespace_release(group);
    }
    return result;
}


/* EN/RU: Submit the captured guest path with an empty payload.
 * Отправить путь гостя с пустыми стартовыми данными. */
void Vfsnative_run(void *frame)
{
    F_Vfsnative_run call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_run, ret), sizeof(*call.ret));
    char name[DVM_NS_PATH_LIMIT];
    WORD result = path(call.path, name) ? run_path(name, NULL, 0) : DVM_LAUNCH_NOT_SENT;
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Copy binary startup data before releasing managed admission.
 * Скопировать бинарные стартовые данные до выхода из допуска VM. */
void Vfsnative_runargs(void *frame)
{
    F_Vfsnative_runargs call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_runargs, ret), sizeof(*call.ret));
    Array *arguments = call.arguments;
    WORD result = DVM_LAUNCH_NOT_SENT;
    char name[DVM_NS_PATH_LIMIT];
    if (!path(call.path, name))
    {
        dvm_sys_scalar_result(&result, call.ret, sizeof(result));
        return;
    }
    if (arguments == H || arguments == nil)
    {
        result = run_path(name, NULL, 0);
    }
    else if (D2H(arguments)->t == &Tarray && arguments->t && arguments->t != H &&
        arguments->t->size == 1 && !arguments->t->np && arguments->len >= 0 &&
        arguments->len <= DVM_LAUNCH_ARGUMENT_LIMIT)
    {
        size_t size = (size_t)arguments->len;
        void *copy = size ? malloc(size) : NULL;
        if (!size || (copy && arguments->data && arguments->data != H))
        {
            if (size)
            {
                memcpy(copy, arguments->data, size);
            }
            result = run_path(name, copy, size);
        }
        free(copy);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Register scalar module before guest publication; generated ABI stays build-owned.
 * Зарегистрировать scalar module до публикации guests; generated ABI принадлежит сборке. */
void dvm_vfsnative_initialize(void)
{
    builtinmod("$Vfsnative", Vfsnativemodtab, Vfsnativemodlen);
}
