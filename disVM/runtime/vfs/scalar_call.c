/* Copyright (c) 2026 bhgv
 * [GC:] EN: Capture one scalar Dis call and exchange S-records in its native wait scope.
 * RU: Захватить скалярный вызов Dis и обменяться S-записями в области нативного ожидания.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "kernel.h"
#include "loader.h"
#include "scalar_files.h"
#include "fns.h"
#include "interp.h"
#include "runt.h"
#include "launch_files.h"
#include "runqueue.h"
#include "scalar_direct.h"
#include "managed_value.h"
#include "../../native/terminal/scalar_function.h"
#include "scalar_owner_internal.h"
#include "raise.h"
#include "guest.h"
#include "resource.h"
#include "native_wait.h"
#include "path_cancel.h"
#include "cancel.h"
#include "runtime_gc.h"
#include "sys_capture.h"
#include "vfsnative_decl.h"
#include "../../native/terminal/scalar_route.h"
#include "../../native/terminal/scalar_verify.h"
#include "../../native/terminal/record_write.h"
#include "../../native/terminal/call_scope.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char scope_failed[] = "remote caller namespace unavailable"; /* Refuse the call before executing Dis. */
static const char session_end[] = "scalar session closed"; /* Clean EOF or exact close record. */
static const char session_idle[] = "scalar session idle"; /* Nonblocking empty read. */
static const char owner_error[] = "remote scalar owner error"; /* Verified reply, route remains synchronized. */
#define SCALAR_IDLE 0UL /* Native route has no call in progress. */
#define SCALAR_BUSY 1UL /* A caller owns the one outstanding exchange. */
#define SCALAR_RETIRED 2UL /* Prior exchange ended without a verified reply. */


/* EN/RU: Resolve one provider in the retained namespace and capture its exact node.
 * Выбрать провайдер в удерживаемом пространстве и захватить точный узел. */
static DVM_Provider *capture(DVM_Namespace *group, const char *path, unsigned access)
{
    DVM_ResourcePlan plan = {0};
    if (!dvm_resource_plan(group, path, 0, &plan))
    {
        return NULL;
    }
    plan.requested_access = access;
    FVM_FileInfo info;
    DVM_Provider *provider = NULL;
    if (dvm_capture_cancellable(&plan) && dvm_resource_info(&plan, &info) == FVM_OK && !info.directory && plan.provider &&
        (dvm_provider_native_flags(plan.provider) & DVM_PROVIDER_CANCEL_PATH))
    {
        provider = dvm_provider_capture(plan.provider, plan.relative);
    }
    dvm_resource_close(&plan);
    return provider;
}


typedef enum ScalarRead
{
    READ_REPLY, /* Bounded wait for the caller's result. */
    READ_CALL, /* Bounded wait for one owner request. */
    READ_POLL, /* One read of a local session mailbox. */
    READ_SESSION /* Wait for a request, orderly close or carrier EOF. */
} ScalarRead;


/* EN/RU: Read a complete reply, repeating only empty pending reads for up to 3 seconds.
 * Прочитать полный ответ, повторяя только пустые чтения до трёх секунд. */
static const char *receive(DVM_VfsHandle *reply, unsigned long tag, int64_t *value, ScalarRead mode, DVM_OwnerDecode decoder, void *decoded, int *required)
{
    struct timespec start;
    if (mode != READ_SESSION && mode != READ_POLL && clock_gettime(CLOCK_MONOTONIC, &start))
    {
        return "remote scalar reply unavailable";
    }
    while (!dvm_cancel_requested(dvm_cancel_current()))
    {
        size_t capacity = tag == STYX_MANAGED_REQUEST_TAG ?
            STYX_MANAGED_LIST_LIMIT + STYX_SCOPED_LARGE_OVERHEAD + 1 :
            STYX_FUNCTION_FRAME_CAPACITY + STYX_SCOPED_CALL_OVERHEAD + 1;
        unsigned char bytes[capacity];
        DVM_IoResult got = dvm_vfs_read_chunk(reply, bytes, sizeof(bytes));
        if ((mode == READ_SESSION || mode == READ_POLL) && got.status == FVM_OK && (got.flags & DVM_IO_PREFIX) &&
            !(got.flags & DVM_IO_UNCERTAIN) &&
            ((!got.count && got.stop == DVM_IO_EOF) || styxscalarclosed(tag, bytes, got.count)))
        {
            return session_end;
        }
        if (got.stop != DVM_IO_AGAIN || got.count || (got.flags & DVM_IO_UNCERTAIN))
        {
            if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) || (got.flags & DVM_IO_UNCERTAIN))
            {
                return "remote scalar reply unavailable";
            }
            if (decoder)
            {
                return dvm_scalar_owner_decode(decoder, tag, bytes, got.count, decoded, required) ? NULL : "invalid typed owner request";
            }
            StyxScalarKind kind;
            if (!styxscalarread(tag, bytes, got.count, &kind, value))
            {
                return "remote scalar reply unavailable";
            }
            if (mode != READ_REPLY)
            {
                return kind == STYX_SCALAR_CALL ? NULL : "invalid scalar request";
            }
            return kind == STYX_SCALAR_VALUE ? NULL :
                kind == STYX_SCALAR_ERROR ? owner_error : "invalid scalar reply";
        }
        if (mode == READ_POLL)
        {
            return session_idle;
        }
        struct timespec now;
        if (mode != READ_SESSION && (clock_gettime(CLOCK_MONOTONIC, &now) ||
            (now.tv_sec - start.tv_sec) * INT64_C(1000) + (now.tv_nsec - start.tv_nsec) / 1000000 >= 3000))
        {
            break;
        }
        const struct timespec pause = {0, 10000000};
        nanosleep(&pause, NULL);
    }
    return "remote scalar reply unavailable";
}


/* EN/RU: Use copied native arguments only; open the reply before sending.
 * Работать со скопированными аргументами. Открыть ответ до отправки. */
static const char *exchange(DVM_Provider *request, DVM_Provider *reply,
    unsigned long request_tag, unsigned long reply_tag, int64_t argument, int64_t *value)
{
    DVM_VfsHandle handle = {0};
    const char *failure = "remote scalar call not sent";
    if (request && reply && (dvm_provider_native_flags(reply) & DVM_PROVIDER_CANCEL_PATH) &&
        dvm_provider_open(reply, ".", "r", &handle) == FVM_OK && (handle.native_flags & DVM_VFS_CANCEL_DATA))
    {
        DVM_LaunchDelivery sent = styxscalarcallfile(request, request_tag, argument);
        if (sent == DVM_LAUNCH_DELIVERED)
        {
            failure = receive(&handle, reply_tag, value, READ_REPLY, NULL, NULL, NULL);
        }
        else if (sent == DVM_LAUNCH_UNKNOWN)
        {
            failure = "remote scalar call outcome unknown";
        }
    }
    dvm_vfs_close(&handle);
    return failure;
}


/* EN/RU: Exchange a copied scalar outside admission and publish once after reentry.
 * Передать скопированный скаляр вне допуска и записать результат после возврата. */
static const char *scalar_value(DVM_Namespace *group, const char *request, const char *reply,
    unsigned long request_tag, unsigned long reply_tag, WORD argument, WORD *result, DVM_ScalarFiles *files)
{
    Prog *process = currun();
    uint32_t pid = process && process->pid > 0 ? (uint32_t)process->pid : 0;
    DVM_LaunchFiles *descriptors = process ? ((DVM_RuntimeProcess *)process)->launch_files : NULL;
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    if (released < 0)
    {
        return "remote scalar call not sent";
    }
    int64_t value = 0;
    DVM_Provider *writer = files ? files->request : capture(group, request, FVM_ACCESS_WRITE);
    DVM_Provider *reader = files ? files->reply : capture(group, reply, FVM_ACCESS_READ);
    int duplex = files && files->duplex;
    int outcome = duplex ? dvm_scalar_function_invoke(files, group, descriptors, pid, argument, &value) : 0;
    const char *failure = duplex ? (outcome == 1 ? NULL : outcome == 2 ? owner_error : outcome < 0 ? "remote scalar call not sent" : "remote scalar call outcome unknown") :
        exchange(writer, reader, request_tag, reply_tag, argument, &value);
    if (!files)
    {
        dvm_provider_release(writer);
        dvm_provider_release(reader);
    }
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || failure)
    {
        return cancelled ? "remote scalar call cancelled" : failure;
    }
    WORD narrowed = (WORD)value;
    if ((int64_t)narrowed != value)
    {
        return "invalid scalar reply";
    }
    *result = narrowed;
    return NULL;
}


/* [GC:] EN/RU: Capture the compiled builtin frame, perform native exchange, publish one scalar.
 * Захватить скомпилированный кадр builtin, выполнить обмен и записать скаляр. */
void Vfsnative_scalarcall(void *frame)
{
    F_Vfsnative_scalarcall call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarcall, ret), sizeof(*call.ret));
    char request[DVM_NS_PATH_LIMIT], reply[DVM_NS_PATH_LIMIT];
    DVM_Namespace *group = dvm_namespace_current_group();
    if (!group || call.requesttag <= 0 || call.requesttag > UINT32_MAX ||
        call.replytag <= 0 || call.replytag > UINT32_MAX ||
        !dvm_scalar_copy_path(call.request, request) || !dvm_scalar_copy_path(call.reply, reply))
    {
        error("remote scalar call not sent");
    }
    WORD result = 0;
    const char *failure = scalar_value(group, request, reply, (unsigned long)call.requesttag,
        (unsigned long)call.replytag, call.value, &result, NULL);
    if (failure)
    {
        error((char *)failure);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Read from the checked owner inbox outside managed admission.
 * Читать из проверенного приёмника владельца вне допуска VM. */
static const char *take_request(String *name, int64_t tag, ScalarRead mode, WORD *result,
    DVM_OwnerDecode decoder, void *decoded, Prog *scoped)
{
    char path[DVM_NS_PATH_LIMIT];
    DVM_Namespace *group = dvm_namespace_current_group();
    if (!group || tag <= 0 || tag > UINT32_MAX || !dvm_scalar_copy_path(name, path))
    {
        error("scalar owner route unavailable");
    }
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    if (released < 0)
    {
        error("scalar owner route unavailable");
    }
    DVM_Provider *provider = capture(group, path, FVM_ACCESS_READ);
    DVM_VfsHandle handle = {0};
    int64_t value = 0;
    const char *failure = "scalar owner route unavailable";
    if (provider && (dvm_provider_native_flags(provider) & DVM_PROVIDER_CANCEL_PATH) &&
        dvm_provider_open(provider, ".", "r", &handle) == FVM_OK && (handle.native_flags & DVM_VFS_CANCEL_DATA))
    {
        int required = 0;
        failure = receive(&handle, (unsigned long)tag, &value, mode, decoder, decoded, scoped ? &required : NULL);
        if (!failure && required && !dvm_scalar_caller_begin(scoped, group, &handle))
        {
            failure = scope_failed;
        }
    }
    dvm_vfs_close(&handle);
    dvm_provider_release(provider);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || failure)
    {
        dvm_scalar_caller_end(scoped);
        return cancelled ? "scalar owner request cancelled" : failure;
    }
    if (result)
    {
        *result = (WORD)value;
    }
    return NULL;
}


typedef struct ManagedOwnerInput
{
    unsigned char bytes[STYX_MANAGED_LIST_LIMIT]; /* Copied complete request. */
    size_t size; /* Exact encoded length. */
} ManagedOwnerInput;


/* EN: Copy a complete record before the local inbox releases its storage.
 * RU: Скопировать полную запись до освобождения памяти локальным приёмником. */
static int capture_managed(unsigned long tag, const void *bytes, size_t size, void *state)
{
    ManagedOwnerInput *input = state;
    FVM_SRecord record;
    size_t used;
    if (size > sizeof(input->bytes) ||
        fvm_srecord_read(bytes, size, sizeof(input->bytes), &record, &used) != FVM_S_OK ||
        used != size || (record.tag != tag && record.tag != STYX_MANAGED_DELTA_REQUEST_TAG &&
            record.tag != STYX_MANAGED_SPAN_REQUEST_TAG && record.tag != STYX_MANAGED_RUN_REQUEST_TAG))
    {
        return 0;
    }
    memcpy(input->bytes, bytes, size);
    input->size = size;
    return 1;
}


/* [GC:] EN: Receive a validated local record and arm its scope for the caller's worker.
 * RU: Принять проверенную локальную запись и подготовить контекст её исполнителя. */
void Vfsnative_scalarmanagednext(void *frame)
{
    F_Vfsnative_scalarmanagednext call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarmanagednext, ret), sizeof(*call.ret));
    Prog *process = currun();
    ManagedOwnerInput input;
    const char *failure = take_request(call.path, STYX_MANAGED_REQUEST_TAG, READ_SESSION, NULL,
        capture_managed, &input, process);
    Array *wire = H;
    if (waserror())
    {
        dvm_scalar_caller_end(process);
        nexterror();
    }
    if (!failure)
    {
        if (!dvm_scalar_arm_handoff())
        {
            error("managed owner call requires a private reply");
        }
        wire = mem2array(input.bytes, (int)input.size);
        Setmark(D2H(wire));
    }
    else if (failure != session_end)
    {
        error((char *)failure);
    }
    Array *previous = dvm_sys_owner(call.ret);
    memcpy(call.ret, &wire, sizeof(wire));
    destroy(previous);
    poperror();
}


/* EN: Publish one failure for the current private call, if it is still open.
 * RU: Отправить ошибку текущего частного вызова, если он ещё открыт. */
void Vfsnative_scalarmanagederror(void *frame)
{
    (void)frame;
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (owner && owner->remote_call_reply)
    {
        dvm_styx_owner_finish(NULL, NULL, NULL, NULL, 0);
    }
}


/* EN/RU: Read one admitted scalar request from the active owner's namespace.
 * Прочитать один допущенный скалярный запрос из пространства владельца. */
void Vfsnative_scalartake(void *frame)
{
    F_Vfsnative_scalartake call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalartake, ret), sizeof(*call.ret));
    WORD result = 0;
    const char *failure = take_request(call.path, call.tag, READ_CALL, &result, NULL, NULL, NULL);
    if (failure)
    {
        error((char *)failure);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN: Publish the shared two-cell session result for either signed width.
 * RU: Выдать общий результат сессии из двух ячеек для обеих ширин. */
static void next_result(String *path, LONG tag, void *ret)
{
    WORD result[2] = {0};
    const char *failure = take_request(path, tag, READ_SESSION, &result[1], NULL, NULL, NULL);
    if (failure && failure != session_end)
    {
        error((char *)failure);
    }
    result[0] = !failure;
    dvm_sys_scalar_result(result, ret, sizeof(result));
}


/* EN/RU: Read the next session request or return clean end of session.
 * Прочитать следующий запрос сессии или вернуть признак её завершения. */
void Vfsnative_scalarnext(void *frame)
{
    F_Vfsnative_scalarnext call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarnext, ret), 2 * sizeof(WORD));
    next_result(call.path, call.tag, call.ret);
}


/* EN/RU: Read the local mailbox once and distinguish idle from EOF.
 * Прочитать локальный приёмник один раз и отличить ожидание от EOF. */
void Vfsnative_scalarpoll(void *frame)
{
    F_Vfsnative_scalarpoll call;
    WORD result[2] = {0, 0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarpoll, ret), sizeof(result));
    const char *failure = take_request(call.path, call.tag, READ_POLL, &result[1], NULL, NULL, NULL);
    if (failure && failure != session_idle && failure != session_end)
    {
        error((char *)failure);
    }
    result[0] = !failure ? 1 : failure == session_idle ? -1 : 0;
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Read the full-width mailbox once and distinguish idle from EOF.
 * Прочитать локальный приёмник один раз и отличить ожидание от EOF. */
void Vfsnative_scalarbigpoll(void *frame)
{
    F_Vfsnative_scalarbigpoll call;
    WORD result[2] = {0, 0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarbigpoll, ret), sizeof(result));
    const char *failure = take_request(call.path, call.tag, READ_POLL, &result[1], NULL, NULL, NULL);
    if (failure && failure != session_idle && failure != session_end)
    {
        error((char *)failure);
    }
    result[0] = !failure ? 1 : failure == session_idle ? -1 : 0;
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN: Send a complete pre-encoded owner result through the selected mailbox.
 * RU: Отправить полную закодированную запись в приёмник владельца. */
void dvm_scalar_owner_record(String *name, const void *bytes, size_t used, int scoped)
{
    Prog *process = scoped ? currun() : NULL;
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    DVM_Provider *reply = owner ? owner->remote_call_reply : NULL;
    if (owner)
    {
        owner->remote_call_reply = NULL;
        dvm_scalar_caller_end(process);
    }
    char path[DVM_NS_PATH_LIMIT];
    DVM_Namespace *group = dvm_namespace_current_group();
    if (!reply && (!group || !dvm_scalar_copy_path(name, path)))
    {
        error("scalar owner route unavailable");
    }
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    if (released < 0)
    {
        dvm_provider_release(reply);
        error("scalar owner route unavailable");
    }
    DVM_Provider *provider = reply ? reply : capture(group, path, FVM_ACCESS_WRITE);
    DVM_LaunchDelivery delivery = styxterminalrecordwrite(provider, bytes, used);
    dvm_provider_release(provider);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || delivery != DVM_LAUNCH_DELIVERED)
    {
        error("scalar owner reply outcome unknown");
    }
}


/* EN/RU: Receive a regular typed Dis call and return through its original frame.
 * Принять обычный типизированный вызов Dis и вернуть число через исходный кадр. */
void dvm_scalar_entry(void *frame, unsigned index)
{
    DirectFrame call;
    Module *module = DVM_R.M->m;
    DVM_ScalarFiles *files = module->scalar_files;
    unsigned ordinal = index;
    for (; files && index; index--)
    {
        files = files->next;
    }
    if (index)
    {
        error(exModule);
    }
    Type *expected_type = module->ext[ordinal].frame;
    if (files && (files->signature == DVM_REF_PAIR_SIGNATURE || files->ref_profile == STYX_REF_PAIR_REQUEST_TAG))
    {
        dvm_ref_pair_entry(frame, expected_type, files);
        return;
    }
    if (files && ((files->dynamic && !files->ref_profile) || files->signature == DVM_CALL_LIST_SIGNATURE))
    {
        if (files->explicit_end == STYX_CALL_RECORD_FRAMING)
        {
            dvm_managed_call_entry(frame, expected_type, files);
        }
        else
        {
            dvm_call_list_entry(frame, expected_type, files);
        }
        return;
    }
    if (files && (files->signature == DVM_REF_SIGNATURE || files->ref_profile == STYX_REF_REQUEST_TAG))
    {
        dvm_ref_entry(frame, expected_type, files);
        return;
    }
    if (files && files->signature == DVM_TUPLE_SIGNATURE)
    {
        dvm_tuple_entry(frame, expected_type, files);
        return;
    }
    uintptr_t address = (uintptr_t)frame;
    uintptr_t end = (uintptr_t)DVM_R.SP;
    if (!frame || frame == H || frame != (void *)DVM_R.FP || address > end || end - address < sizeof(call))
    {
        error(exBounds);
    }
    Type *frame_type;
    memcpy(&frame_type, (unsigned char *)frame + offsetof(Frame, t), sizeof(frame_type));
    if (frame_type && frame_type != expected_type)
    {
        error(exModule);
    }
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(DirectFrame, ret), sizeof(*call.ret));
    unsigned long request_tag = module->dev;
    unsigned long reply_tag = module->qid.vers;
    DVM_Namespace *group = dvm_namespace_current_group();
    if (!group)
    {
        error("remote scalar call not sent");
    }
    unsigned long independent = SCALAR_IDLE;
    unsigned long *state = !files ? &module->mtime : files->duplex ? &independent : &files->state;
    ulong expected = SCALAR_IDLE;
    if (!__atomic_compare_exchange_n(state, &expected, SCALAR_BUSY, 0,
            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    {
        error(expected == SCALAR_BUSY ? "remote scalar route busy" : "remote scalar route retired");
    }
    if (waserror())
    {
        __atomic_store_n(state, SCALAR_RETIRED, __ATOMIC_RELEASE);
        nexterror();
    }
    WORD result = 0;
    const char *failure = scalar_value(group, "/scalar/request", "/scalar/reply",
        request_tag, reply_tag, call.value, &result, files);
    __atomic_store_n(state, !failure || failure == owner_error ? SCALAR_IDLE : SCALAR_RETIRED,
        __ATOMIC_RELEASE);
    poperror();
    if (failure)
    {
        error((char *)failure);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: See scalar_owner_internal.h. / См. scalar_owner_internal.h. */
int dvm_scalar_owner_next(String *path, unsigned long tag, DVM_OwnerDecode decoder, void *decoded, int scoped)
{
    DVM_RuntimeProcess *owner = scoped ? (DVM_RuntimeProcess *)currun() : NULL;
    if (scoped && (!owner || owner->remote_call_group))
    {
        error("invalid scalar owner scope");
    }
    const char *failure = take_request(path, tag, READ_SESSION, NULL, decoder, decoded, (Prog *)owner);
    if (failure == scope_failed)
    {
        return -1;
    }
    if (failure && failure != session_end)
    {
        error((char *)failure);
    }
    return !failure;
}


/* EN: Decode one full-width session request using the shared scalar record path.
 * RU: Прочитать полноразрядный запрос через общий скалярный путь. */
void Vfsnative_scalarbignext(void *frame)
{
    F_Vfsnative_scalarbignext call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarbignext, ret), 2 * sizeof(WORD));
    next_result(call.path, call.tag, call.ret);
}
