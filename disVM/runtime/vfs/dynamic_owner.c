/* Copyright (c) 2026 bhgv
 * EN: Install the local call scope and move scalar words through the owner mailbox.
 * RU: Установить локальный контекст вызова и передать скаляры через приёмник владельца.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "namespace.h"
#include "runqueue.h"
#include "boot.h"
#include "export.h"
#include "program_view.h"
#include "sysevents.h"
#include "cancel.h"
#include "channel_proxy.h"
#include "flashvm/srecord.h"
#include "sys_capture.h"
#include "scalar_owner_internal.h"
#include "../../native/terminal/scalar_function.h"
#include "vfsnative_decl.h"
#include "raise.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/call_scope.h"
#include "../../native/terminal/load_profile.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/tuple_record.h"
#include "../../native/terminal/unibus_inbox.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>


/* EN: Install the boot VM's contextual device in this private native group.
 * RU: Установить контекстное устройство начальной VM в частный namespace. */
static int install_events(DVM_Namespace *scope)
{
    const char *path = "/dev/sysevents";
    DVM_ResourcePlan source = {0};
    DVM_NamespacePlan target;
    if (!dvm_resource_plan(dvm_namespace_boot_group(), path, 0, &source))
    {
        return 0;
    }
    int installed = dvm_sysevents_is_device(source.provider) &&
        dvm_namespace_plan(scope, path, &target) &&
        dvm_namespace_mount_provider(scope, &target, source.provider, "/system-events") == 1;
    dvm_resource_close(&source);
    return installed;
}


/* EN: Clone the borrowed owner's namespace and overlay only the pinned caller.
 * Return a private owned namespace or NULL before any Dis call runs.
 * RU: Копировать namespace владельца и наложить только закреплённого клиента.
 * Вернуть свою копию либо NULL до выполнения Dis. */
static DVM_Namespace *caller_namespace(DVM_Namespace *group, DVM_Provider *context, uint32_t pid)
{
    DVM_Provider *projection = dvm_program_aliases(context, pid);
    DVM_Namespace *copy = projection ? dvm_namespace_copy(group) : NULL;
    int installed = copy && dvm_program_mount_private(copy, projection, "/.prog", NULL) && install_events(copy);
    dvm_provider_release(projection);
    if (!installed)
    {
        dvm_namespace_release(copy);
        return NULL;
    }
    return copy;
}


/* EN/RU: See scalar_owner_internal.h. / См. scalar_owner_internal.h. */
int dvm_scalar_caller_begin(Prog *process, DVM_Namespace *group, DVM_VfsHandle *handle)
{
    DVM_Provider *caller = handle->ops->take_context ? handle->ops->take_context(handle->state) : NULL;
    DVM_Cancel *cancel = caller ? dvm_provider_call_cancel(caller) : NULL;
    DVM_Provider *reply = dvm_provider_call_reply(caller);
    DVM_StyxCallerKey key = {0};
    int identified = dvm_provider_styx_caller(caller, &key);
    DVM_Namespace *scope = caller && (identified || !reply) ? caller_namespace(group, caller, key.pid) : NULL;
    DVM_SysEvents *events = scope ? dvm_sysevents_create(scope) : NULL;
    if (!events)
    {
        dvm_sysevents_retire(events);
        dvm_sysevents_release(events);
        dvm_namespace_release(scope);
        dvm_provider_release(caller);
        dvm_cancel_release(cancel);
        styxunibusmailboxseal(reply);
        dvm_provider_release(reply);
        return 0;
    }
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    owner->remote_call_key = key;
    owner->remote_call_reply = reply;
    owner->remote_call_cancel_delivered = 0;
    owner->remote_call_cancel = cancel;
    owner->remote_call_group = scope;
    owner->remote_call_context = caller;
    owner->remote_call_events = events;
    return 1;
}


/* EN/RU: See scalar_owner_internal.h. / См. scalar_owner_internal.h. */
void dvm_scalar_caller_end(Prog *process)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    if (!owner || !owner->remote_call_group)
    {
        return;
    }
    dvm_channel_route_end(&owner->remote_channels);
    DVM_Namespace *scope = owner->remote_call_group;
    DVM_SysEvents *events = owner->remote_call_events;
    dvm_cancel_release(owner->remote_call_cancel);
    owner->remote_call_cancel = NULL;
    owner->remote_call_key = (DVM_StyxCallerKey){0};
    owner->remote_call_handoff = 0;
    styxunibusmailboxseal(owner->remote_call_reply);
    dvm_provider_release(owner->remote_call_reply);
    owner->remote_call_reply = NULL;
    owner->remote_call_group = NULL;
    dvm_provider_release(owner->remote_call_context);
    owner->remote_call_context = NULL;
    owner->remote_call_events = NULL;
    dvm_sysevents_retire(events);
    dvm_sysevents_release(events);
    dvm_namespace_release(scope);
}


/* EN/RU: See scalar_owner_internal.h. / См. scalar_owner_internal.h. */
int dvm_scalar_owner_decode(DVM_OwnerDecode decode, unsigned long tag, const void *bytes, size_t size,
    void *decoded, int *required)
{
    FVM_SRecord envelope;
    size_t used;
    size_t capacity = tag == STYX_MANAGED_REQUEST_TAG ?
        STYX_MANAGED_LIST_LIMIT + STYX_SCOPED_LARGE_OVERHEAD :
        STYX_FUNCTION_FRAME_CAPACITY + STYX_SCOPED_CALL_OVERHEAD;
    if (required && fvm_srecord_read(bytes, size, capacity,
        &envelope, &used) == FVM_S_OK && used == size && envelope.tag == STYX_LOCAL_CONTEXT_TAG)
    {
        *required = 1;
        return decode(tag, envelope.payload, envelope.size, decoded);
    }
    return decode(tag, bytes, size, decoded);
}


/* EN: Read the dispatcher's complete four-word local argument record.
 * RU: Прочитать полную локальную запись из четырёх ячеек аргументов. */
static int decode_words(unsigned long tag, const void *bytes, size_t size, void *result)
{
    return styxcallrecordread(tag, "(bbbb)", bytes, size, result);
}


/* EN: Return EOF or the four staged input cells to the owner process.
 * RU: Вернуть EOF либо четыре скопированные входные ячейки процессу владельца. */
static void scalar_raw_next(void *frame, int handoff)
{
    F_Vfsnative_scalarrawnext call;
    WORD result[STYX_DYNAMIC_CELLS + 1];
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarrawnext, ret), sizeof(result));
    StyxCallValues decoded = {0};
    result[0] = dvm_scalar_owner_next(call.path, STYX_DYNAMIC_REQUEST_TAG, decode_words, &decoded, 1);
    if (handoff && result[0] > 0 && dvm_scalar_arm_handoff())
    {
        result[0] = 2;
    }
    for (size_t i = 0; i < STYX_DYNAMIC_CELLS; i++)
    {
        result[i + 1] = decoded.cells[i];
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: See scalar_owner_internal.h. / См. scalar_owner_internal.h. */
int dvm_scalar_arm_handoff(void)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (!owner || !owner->remote_call_reply || owner->remote_call_handoff)
    {
        return 0;
    }
    owner->remote_call_handoff = 1;
    owner->remote_call_cancel_delivered = 1;
    return 1;
}


/* EN: Receive on the current worker using the original owner ABI.
 * RU: Принять вызов в текущем исполнителе через исходный ABI владельца. */
void Vfsnative_scalarrawnext(void *frame)
{
    scalar_raw_next(frame, 0);
}


/* EN: Receive a call and reserve its scope for the next dispatcher spawn.
 * RU: Принять вызов и зарезервировать контекст для ближайшего spawn диспетчера. */
void Vfsnative_scalarrawdispatchnext(void *frame)
{
    scalar_raw_next(frame, 1);
}


/* EN: Enable cancellation in the spawned worker after its exception handler is live.
 * RU: Включить отмену после установки обработчика исключений дочернего процесса. */
void Vfsnative_scalarcallstart(void *frame)
{
    (void)frame;
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (!owner || !owner->remote_call_reply || owner->remote_call_handoff != 2)
    {
        error("invalid scalar call worker");
    }
    owner->remote_call_handoff = 0;
    owner->remote_call_cancel_delivered = 0;
    if (dvm_cancel_requested(owner->remote_call_cancel))
    {
        owner->remote_call_cancel_delivered = 1;
        error("remote call cancelled");
    }
}


/* EN: Seal any pending handoff after a failed worker spawn.
 * RU: Закрыть ожидающую передачу при отказе создания исполнителя. */
void Vfsnative_scalarcallabort(void *frame)
{
    (void)frame;
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (owner && owner->remote_call_handoff == 1)
    {
        dvm_scalar_caller_end((Prog *)owner);
    }
    if (owner)
    {
        dvm_scalar_discard_staged((Prog *)owner);
    }
}


/* EN/RU: Detached native ownership, published by one dispatcher to one worker.
 * Отсоединённый контекст одного диспетчера для одного исполнителя. */
typedef struct DVM_ScalarHandoff
{
    DVM_StyxCallerKey key; /* Caller identity. */
    DVM_Namespace *group; /* Owned call namespace. */
    DVM_SysEvents *events; /* Owned namespace event stream. */
    DVM_Provider *context; /* Owned exact caller directory. */
    DVM_Provider *reply; /* Owned private reply mailbox. */
    DVM_Cancel *cancel; /* Owned fid cancellation token. */
} DVM_ScalarHandoff;


/* EN/RU: See scalar_owner_internal.h. / См. scalar_owner_internal.h. */
void dvm_scalar_discard_staged(Prog *process)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    DVM_ScalarHandoff *stage = __atomic_exchange_n(&owner->remote_call_staged, NULL, __ATOMIC_ACQ_REL);
    if (stage)
    {
        dvm_sysevents_retire(stage->events);
        dvm_sysevents_release(stage->events);
        styxunibusmailboxseal(stage->reply);
        dvm_provider_release(stage->reply);
        dvm_cancel_release(stage->cancel);
        dvm_namespace_release(stage->group);
        dvm_provider_release(stage->context);
        free(stage);
    }
}


/* EN/RU: See scalar_owner_internal.h. / См. scalar_owner_internal.h. */
int dvm_scalar_take_staged(Prog *parent, Prog *child)
{
    DVM_RuntimeProcess *source = (DVM_RuntimeProcess *)parent;
    DVM_RuntimeProcess *target = (DVM_RuntimeProcess *)child;
    if (target->remote_call_group ||
        __atomic_load_n(&source->remote_call_staged_worker, __ATOMIC_ACQUIRE) != child->pid)
    {
        return 0;
    }
    DVM_ScalarHandoff *stage = __atomic_exchange_n(&source->remote_call_staged, NULL, __ATOMIC_ACQ_REL);
    if (!stage)
    {
        return 0;
    }
    target->remote_call_key = stage->key;
    target->remote_call_group = stage->group;
    target->remote_call_context = stage->context;
    target->remote_call_events = stage->events;
    target->remote_call_reply = stage->reply;
    target->remote_call_cancel = stage->cancel;
    target->remote_call_handoff = 2;
    target->remote_call_cancel_delivered = 1;
    free(stage);
    return 1;
}


/* EN: Publish the armed scope for one already running child and detach it.
 * RU: Передать подготовленный контекст существующему потомку и отсоединить его. */
void Vfsnative_scalarcallstage(void *frame)
{
    F_Vfsnative_scalarcallstage call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memcpy(&call, frame, sizeof(call));
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (!owner || owner->remote_call_handoff != 1 || call.worker <= 0 ||
        call.worker > INT_MAX || __atomic_load_n(&owner->remote_call_staged, __ATOMIC_ACQUIRE))
    {
        error("invalid scalar call handoff");
    }
    DVM_ScalarHandoff *stage = malloc(sizeof(*stage));
    if (!stage)
    {
        error(exNomem);
    }
    *stage = (DVM_ScalarHandoff){owner->remote_call_key, owner->remote_call_group, owner->remote_call_events,
        owner->remote_call_context, owner->remote_call_reply, owner->remote_call_cancel};
    owner->remote_call_key = (DVM_StyxCallerKey){0};
    owner->remote_call_group = NULL;
    owner->remote_call_context = NULL;
    owner->remote_call_events = NULL;
    owner->remote_call_reply = NULL;
    owner->remote_call_cancel = NULL;
    owner->remote_call_handoff = 0;
    __atomic_store_n(&owner->remote_call_staged_worker, (int)call.worker, __ATOMIC_RELEASE);
    __atomic_store_n(&owner->remote_call_staged, stage, __ATOMIC_RELEASE);
}


extern int dvm_runtime_scalar_take(Prog *child, int source_pid);


/* EN: Install the detached scope on the worker before enabling cancellation.
 * RU: Установить отсоединённый контекст до включения отмены. */
void Vfsnative_scalarcalltake(void *frame)
{
    (void)frame;
    if (!dvm_runtime_scalar_take(currun(), 0))
    {
        error("scalar call handoff missing");
    }
}


/* EN: Collect an explicitly addressed handoff from the specified dispatcher.
 * RU: Принять адресованную этому исполнителю передачу от указанного диспетчера. */
void Vfsnative_scalarcalltakefrom(void *frame)
{
    F_Vfsnative_scalarcalltakefrom call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memcpy(&call, frame, sizeof(call));
    if (call.source <= 0 || call.source > INT_MAX || !dvm_runtime_scalar_take(currun(), (int)call.source))
    {
        error("scalar call handoff missing");
    }
}


/* EN: Copy the full session identity and checked PID to the dispatcher.
 * RU: Скопировать полную идентичность сессии и проверенный PID диспетчеру. */
void Vfsnative_scalarcallkey(void *frame)
{
    F_Vfsnative_scalarcallkey call;
    WORD fields[5] = {0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarcallkey, ret), sizeof(fields));
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (owner && owner->remote_call_handoff == 1)
    {
        const DVM_StyxCallerKey *key = &owner->remote_call_key;
        for (size_t i = 0; i < 2; i++)
        {
            fields[i] = (uint64_t)key->session.domain[2*i] << 32 | key->session.domain[2*i+1];
            fields[i+2] = (uint64_t)key->session.object[2*i] << 32 | key->session.object[2*i+1];
        }
        fields[4] = key->pid;
    }
    dvm_sys_scalar_result(fields, call.ret, sizeof(fields));
}


/* EN: Return only the validated prefix from the owner's scalar staging area.
 * RU: Вернуть только проверенный префикс из скалярного буфера владельца. */
void Vfsnative_scalarrawreply(void *frame)
{
    F_Vfsnative_scalarrawreply call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memcpy(&call, frame, sizeof(call));
    if (call.count < 0 || call.count > STYX_DYNAMIC_CELLS)
    {
        error("invalid remote scalar result extent");
    }
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t used;
    int encoded;
    if (call.success == 1)
    {
        const LONG fields[] = {call.value.t0, call.value.t1, call.value.t2, call.value.t3};
        StyxCallValues staged = {.count = STYX_DYNAMIC_CELLS};
        for (size_t i = 0; i < (size_t)call.count; i++)
        {
            staged.cells[i] = fields[i];
        }
        encoded = styxcallrecordwrite(STYX_DYNAMIC_REPLY_TAG, "(bbbb)", &staged, bytes, sizeof(bytes), &used);
    }
    else
    {
        encoded = styxscalarwrite(STYX_DYNAMIC_REPLY_TAG, STYX_SCALAR_ERROR, 0, bytes, sizeof(bytes), &used);
    }
    if (!encoded)
    {
        error("invalid remote scalar reply");
    }
    dvm_scalar_owner_record(call.path, bytes, used, 1);
}


/* EN: Query the cancellation token bound to the caller's active function fid.
 * RU: Проверить токен отмены, привязанный к fid текущего вызова. */
void Vfsnative_callcancelled(void *frame)
{
    F_Vfsnative_callcancelled call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_callcancelled, ret), sizeof(*call.ret));
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    WORD result = owner && dvm_cancel_requested(owner->remote_call_cancel);
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}
