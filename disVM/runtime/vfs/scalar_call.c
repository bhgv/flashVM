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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern char *dvm_runtime_file_path(String *name); /* Borrowed path while the calling frame is rooted. */


static const char session_end[] = "scalar session closed"; /* Clean EOF or exact close record. */
static const char owner_error[] = "remote scalar owner error"; /* Verified reply, route remains synchronized. */
#define SCALAR_IDLE 0UL /* Native route has no call in progress. */
#define SCALAR_BUSY 1UL /* A caller owns the one outstanding exchange. */
#define SCALAR_RETIRED 2UL /* Prior exchange ended without a verified reply. */


/* EN/RU: Copy a validated guest spelling before releasing managed admission.
 * Скопировать проверенный путь гостя до выхода из допуска VM. */
static int copy_path(String *name, char output[DVM_NS_PATH_LIMIT])
{
    char *path = dvm_runtime_file_path(name);
    if (!path || !*path || strlen(path) >= DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    strcpy(output, path);
    return 1;
}


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
    READ_SESSION /* Wait for a request, orderly close or carrier EOF. */
} ScalarRead;


/* EN/RU: Read a complete reply, repeating only empty pending reads for up to 3 seconds.
 * Прочитать полный ответ, повторяя только пустые чтения до трёх секунд. */
static const char *receive(DVM_VfsHandle *reply, unsigned long tag, int64_t *value, ScalarRead mode)
{
    struct timespec start;
    if (mode != READ_SESSION && clock_gettime(CLOCK_MONOTONIC, &start))
    {
        return "remote scalar reply unavailable";
    }
    while (!dvm_cancel_requested(dvm_cancel_current()))
    {
        unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY + 1];
        DVM_IoResult got = dvm_vfs_read_chunk(reply, bytes, sizeof(bytes));
        if (mode == READ_SESSION && got.status == FVM_OK && (got.flags & DVM_IO_PREFIX) &&
            !(got.flags & DVM_IO_UNCERTAIN) &&
            ((!got.count && got.stop == DVM_IO_EOF) || styxscalarclosed(tag, bytes, got.count)))
        {
            return session_end;
        }
        if (got.stop != DVM_IO_AGAIN || got.count || (got.flags & DVM_IO_UNCERTAIN))
        {
            StyxScalarKind kind;
            if (got.status == FVM_OK && (got.flags & DVM_IO_PREFIX) && !(got.flags & DVM_IO_UNCERTAIN) &&
                styxscalarread(tag, bytes, got.count, &kind, value))
            {
                if (mode != READ_REPLY)
                {
                    return kind == STYX_SCALAR_CALL ? NULL : "invalid scalar request";
                }
                return kind == STYX_SCALAR_VALUE ? NULL :
                    kind == STYX_SCALAR_ERROR ? owner_error : "invalid scalar reply";
            }
            return "remote scalar reply unavailable";
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
            failure = receive(&handle, reply_tag, value, READ_REPLY);
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
    int outcome = duplex ? dvm_scalar_function_call(files, argument, &value) : 0;
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
        !copy_path(call.request, request) || !copy_path(call.reply, reply))
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
static const char *take_request(String *name, int64_t tag, ScalarRead mode, WORD *result)
{
    char path[DVM_NS_PATH_LIMIT];
    DVM_Namespace *group = dvm_namespace_current_group();
    if (!group || tag <= 0 || tag > UINT32_MAX || !copy_path(name, path))
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
        failure = receive(&handle, (unsigned long)tag, &value, mode);
    }
    dvm_vfs_close(&handle);
    dvm_provider_release(provider);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || failure)
    {
        return cancelled ? "scalar owner request cancelled" : failure;
    }
    WORD narrowed = (WORD)value;
    if ((int64_t)narrowed != value)
    {
        error("invalid scalar request");
    }
    *result = narrowed;
    return NULL;
}


/* EN/RU: Read one admitted scalar request from the active owner's namespace.
 * Прочитать один допущенный скалярный запрос из пространства владельца. */
void Vfsnative_scalartake(void *frame)
{
    F_Vfsnative_scalartake call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalartake, ret), sizeof(*call.ret));
    WORD result = 0;
    const char *failure = take_request(call.path, call.tag, READ_CALL, &result);
    if (failure)
    {
        error((char *)failure);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Read the next session request or return clean end of session.
 * Прочитать следующий запрос сессии или вернуть признак её завершения. */
void Vfsnative_scalarnext(void *frame)
{
    F_Vfsnative_scalarnext call;
    WORD result[2] = {0, 0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarnext, ret), sizeof(result));
    const char *failure = take_request(call.path, call.tag, READ_SESSION, &result[1]);
    if (failure && failure != session_end)
    {
        error((char *)failure);
    }
    result[0] = !failure;
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Send one result or owner error from the active namespace.
 * Отправить результат или ошибку из пространства владельца. */
void Vfsnative_scalarreply(void *frame)
{
    F_Vfsnative_scalarreply call;
    /* The generated void-return frame has no result destination. */
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memmove(&call, frame, sizeof(call));
    char path[DVM_NS_PATH_LIMIT];
    DVM_Namespace *group = dvm_namespace_current_group();
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
    size_t used;
    StyxScalarKind kind = call.success == 1 ? STYX_SCALAR_VALUE : STYX_SCALAR_ERROR;
    if (!group || call.tag <= 0 || call.tag > UINT32_MAX || !copy_path(call.path, path) ||
        !styxscalarwrite((unsigned long)call.tag, kind, call.value, bytes, sizeof(bytes), &used))
    {
        error("scalar owner route unavailable");
    }
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    if (released < 0)
    {
        error("scalar owner route unavailable");
    }
    DVM_Provider *provider = capture(group, path, FVM_ACCESS_WRITE);
    DVM_LaunchDelivery delivery = styxterminalrecordwrite(provider, bytes, used);
    dvm_provider_release(provider);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(released);
    if (cancelled || delivery != DVM_LAUNCH_DELIVERED)
    {
        error("scalar owner reply outcome unknown");
    }
}


/* EN/RU: Hosted compiler frame for one fn(int):int; no managed argument slots.
 * Кадр компилятора hosted для одной fn(int):int, без управляемых аргументов. */
typedef struct DirectFrame
{
    WORD regs[NREG-1]; /* VM linkage cells. */
    WORD *ret; /* Borrowed result slot. */
    unsigned char temps[24]; /* Compiler ABI temporary area. */
    WORD value; /* One scalar argument. */
} DirectFrame;


/* EN/RU: Receive a regular typed Dis call and return through its original frame.
 * Принять обычный типизированный вызов Dis и вернуть число через исходный кадр. */
static void scalar_entry(void *frame)
{
    DirectFrame call;
    Module *module = DVM_R.M->m;
    uintptr_t address = (uintptr_t)frame;
    uintptr_t end = (uintptr_t)DVM_R.SP;
    if (!frame || frame == H || frame != (void *)DVM_R.FP || address > end || end - address < sizeof(call))
    {
        error(exBounds);
    }
    Type *frame_type;
    memcpy(&frame_type, (unsigned char *)frame + offsetof(Frame, t), sizeof(frame_type));
    if (frame_type && frame_type != module->type[0])
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
    DVM_ScalarFiles *files = module->scalar_files;
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


typedef struct DirectBuild
{
    char *path; /* Borrowed logical path through construction. */
    char *name; /* Borrowed checked import name. */
    Module *module; /* Owned partial allocation through kprotect. */
} DirectBuild;


/* EN/RU: Construct one private native module with the compiler's scalar frame.
 * Создать собственный нативный модуль с компилированным скалярным кадром. */
static int build_direct(void *opaque)
{
    DirectBuild *build = opaque;
    Module *module = build->module = newmod(build->path);
    module->type = calloc(1, sizeof(*module->type));
    module->ext = calloc(2, sizeof(*module->ext));
    if (!module->type || !module->ext)
    {
        return 0;
    }
    Type *frame = dtype(freeheap, sizeof(DirectFrame), NULL, 0);
    if (!frame)
    {
        return 0;
    }
    module->type[0] = frame;
    module->ntype = 1;
    runtime(module, module->ext, build->name, (int32_t)DVM_SCALAR_INT_SIGNATURE, scalar_entry, frame);
    return 1;
}


/* EN/RU: Return a private noncached module and reuse per-instance namespace attachment.
 * Вернуть приватный модуль без кэша и использовать привязку пространства экземпляра. */
Module *dvm_scalar_module(char *path, char *name, unsigned long request_tag,
    unsigned long reply_tag, uint64_t revision, DVM_ScalarFiles *files)
{
    DirectBuild build = {path, name, NULL};
    if (!kprotect(build_direct, &build))
    {
        if (build.module)
        {
            unload(build.module);
        }
        kwerrstr("cannot construct remote scalar module");
        return NULL;
    }
    if (!dvm_scalar_files_retain(files))
    {
        unload(build.module);
        kwerrstr("cannot retain scalar file grants");
        return NULL;
    }
    build.module->scalar_files = files;
    /* Private noncached descriptor: these fields carry namespace revision and route tags. */
    build.module->qid.type = DVM_MODULE_ADAPTER_QID;
    build.module->qid.path = revision;
    build.module->qid.vers = reply_tag;
    build.module->dev = request_tag;
    return build.module;
}


/* [GC:] EN: Use the ordinary Dis reader and linker to bind one named export into
 * the generic owner service's scalar slot. The result owns the returned Modlink.
 * RU: Обычный загрузчик и линкер Dis связывают именованный экспорт со скалярным
 * слотом общего сервиса владельца. Возвращённая ссылка владеет Modlink. */
void Vfsnative_scalarload(void *frame)
{
    F_Vfsnative_scalarload call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarload, ret), sizeof(*call.ret));
    char path[DVM_NS_PATH_LIMIT], name[DVM_NS_PATH_LIMIT];
    if (!copy_path(call.path, path) || !copy_path(call.exportname, name) || strlen(name) > 48)
    {
        error("scalar owner implementation unavailable");
    }

    Modlink *empty = H;
    void *previous = dvm_sys_owner(call.ret);
    memmove(call.ret, &empty, sizeof(empty));
    destroy(previous);

    Import imports[] = {{(int32_t)DVM_SCALAR_INT_SIGNATURE, name}, {0, NULL}};
    Module *module = dvm_namespace_read_imports(path, NULL, 1, imports);
    if (!module)
    {
        return;
    }
    if (!module->prog || module->qid.type == DVM_MODULE_ADAPTER_QID)
    {
        unload(module);
        kwerrstr("scalar owner requires a local Dis implementation");
        return;
    }
    Modlink *link = linkmod(module, imports, 1);
    if (link != H)
    {
        Setmark(D2H(link));
        memmove(call.ret, &link, sizeof(link));
    }
}
