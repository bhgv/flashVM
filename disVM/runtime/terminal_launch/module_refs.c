/* Copyright (c) 2026 bhgv
 * EN: Adapt the SDK's module-reference port to an exact guest file in a mixed pair.
 * RU: Адаптация порта ссылок SDK к точному файлу гостя в смешанной паре.
 */
#include "module_refs.h"
#include "entropy.h"
#include "styxrpc/module_ref_port.h"
#include "styxrpc/named_peer.h"
#include "styxrpc/variable_file.h"
#include "caller_styx.h"
#include "client.h"
#include <stdlib.h>
#include <string.h>


struct DvmModuleRefs
{
    struct StyxClient *issuer;                       /* Set once before guest dispatch, owned by the provider. */
    StyxCallerServer *local;                          /* Retained issuer of reverse Qids. */
    unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE]; /* Immutable in both directions. */
};


typedef struct RefCursor
{
    const DvmModuleRefs *service; /* Borrowed through the provider pin of this handle. */
    StyxRpcModuleRefPort *port;   /* Owned for a guest ORDWR open, NULL on the scope view. */
    long position;                /* Byte offset, serialized by the Dis fd. */
} RefCursor;


/* EN: Describe the scope at the exact provider root.
 * RU: Описать область в точном корне провайдера. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, STYXRPC_MODULE_REF_SCOPE_FILE);
    out->length.low = STYXRPC_MODULE_REF_SCOPE_SIZE;
    out->mode = 0666;
    return FVM_OK;
}


/* EN: Report the selected per-open view. Read/write ports carry framed requests.
 * RU: Описать представление выбранного открытия. Порты чтения и записи принимают кадры. */
static FVM_Result cursor_info(void *state, FVM_FileInfo *out)
{
    const RefCursor *cursor = state;
    info(NULL, ".", out);
    if (cursor->port)
    {
        out->length.low = 0;
    }
    else
    {
        out->mode = 0444;
    }
    return FVM_OK;
}


/* EN: Report access for the exact local namespace service.
 * RU: Сообщить доступ к точному сервису локального пространства имён. */
static FVM_Result access(void *state, const char *path, unsigned requested)
{
    (void)state;
    return !strcmp(path, ".") && !(requested & FVM_ACCESS_EXEC) ? FVM_OK : FVM_NOT_FOUND;
}


/* EN: Read a scope fragment or a positioned SDK-port reply into bytes.
 * RU: Прочитать фрагмент области или позиционный ответ порта SDK в bytes. */
static FVM_Result cursor_read(void *state, void *bytes, size_t size, size_t *used)
{
    RefCursor *cursor = state;
    if (cursor->port)
    {
        if (styxrpc_module_ref_port_read(cursor->port, cursor->position, bytes, size, used) != STYXRPC_IO_OK)
        {
            return FVM_ERROR;
        }
    }
    else
    {
        const size_t limit = sizeof(cursor->service->scope);
        size_t left = cursor->position < (long)limit ? limit - (size_t)cursor->position : 0;
        *used = size < left ? size : left;
        if (*used)
        {
            memcpy(bytes, cursor->service->scope + cursor->position, *used);
        }
    }
    cursor->position += (long)*used;
    return FVM_OK;
}


/* EN: Commit one entire SDK request or a four-byte write, advancing only on success.
 * RU: Выполнить полную команду SDK или запись четырёх байтов, сдвигаясь только при успехе. */
static FVM_Result cursor_write(void *state, const void *bytes, size_t size)
{
    RefCursor *cursor = state;
    if (styxrpc_module_ref_port_write(cursor->port, cursor->position, bytes, size) != STYXRPC_IO_OK)
    {
        return FVM_ERROR;
    }
    cursor->position += (long)size;
    return FVM_OK;
}


/* EN: Set a bounded explicit position, in bytes, from the start of the service.
 * RU: Установить ограниченную позицию в байтах от начала сервиса. */
static FVM_Result cursor_seek(void *state, long offset, int origin, long *out)
{
    RefCursor *cursor = state;
    if (origin != FVM_FILE_SEEK_START || offset < 0 ||
        offset > STYXRPC_REF_PORT_FRAME + STYXRPC_MODULE_REF_WIRE_MAX + 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    cursor->position = offset;
    *out = offset;
    return FVM_OK;
}


/* EN: Release the cursor and queue retirement of its exact remote fid, if any.
 * RU: Освободить курсор и поставить в очередь закрытие его точного удалённого fid. */
static void cursor_close(void *state)
{
    RefCursor *cursor = state;
    styxrpc_module_ref_port_close(cursor->port);
    free(cursor);
}


/* EN: Pass one positioned read through the retained local cursor.
 * RU: Передать одно позиционное чтение удерживаемому местному курсору. */
static int local_read(void *file, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    StyxTransfer result;
    styxfileread(file, offset, bytes, capacity, &result);
    *size = result.count;
    return result.success && result.confirmed && !result.uncertain;
}


/* EN: Perform a single positioned write and confirm only a definitive reply.
 * RU: Выполнить одну позиционную запись и подтвердить только определённый ответ. */
static int local_write(void *file, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    StyxTransfer result;
    styxfilewrite(file, offset, bytes, count, &result);
    *size = result.count;
    return result.success && result.confirmed && !result.uncertain;
}


/* EN: Resolve the issued Qid and make an SDK file holding that exact open.
 * RU: Разрешить выданный Qid и передать точное открытие в файл SDK. */
static StyxRpcIoOutcome resolve_local(void *state, const StyxRpcModuleRef *ref, StyxRpcFile *out)
{
    static const StyxRpcVfs ops = {.read = local_read, .write = local_write, .close = styxfileclose};
    StyxOpen *file = styxcalleropen_shared(state, ref->path, ref->node, ref->writable ? 2 : 0);
    StyxMeta info;
    if (!file || !styxfileinfo(file, &info) || info.directory || info.length != STYX_VARIABLE_INT_SIZE)
    {
        styxfileclose(file);
        return STYXRPC_IO_NOT_OPEN;
    }
    *out = (StyxRpcFile){.ops = &ops, .cookie = file,
        .access = STYXRPC_READ | (ref->writable ? STYXRPC_WRITE : 0)};
    return STYXRPC_IO_OK;
}


/* EN: Open an independent scope cursor or the guest's framed SDK port at dot.
 * RU: Открыть независимый курсор области либо порт SDK гостя в точке. */
static FVM_Result open_ref(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps reader = {.info = cursor_info, .read = cursor_read,
        .seek = cursor_seek, .close = cursor_close}; /* Peer capability view. */
    static const DVM_VfsOps proxy = {.info = cursor_info, .read = cursor_read, .write = cursor_write,
        .seek = cursor_seek, .close = cursor_close}; /* Guest binding and value I/O. */
    const DvmModuleRefs *service = state;
    int writable = !strcmp(mode, "rw") || !strcmp(mode, "rwo");
    if (strcmp(path, ".") || (!writable && strcmp(mode, "r")) || (writable && !service->issuer))
    {
        return FVM_UNSUPPORTED;
    }
    RefCursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    cursor->service = service;
    if (writable)
    {
        cursor->port = styxrpc_module_ref_port_open_paired(service->issuer, STYXRPC_MODULE_REF_FORWARD,
            service->scope, service->local ? resolve_local : NULL, service->local);
        if (!cursor->port)
        {
            free(cursor);
            return FVM_ERROR;
        }
    }
    *out = (DVM_VfsHandle){.ops = writable ? &proxy : &reader, .state = cursor};
    return FVM_OK;
}


/* EN: Drop the provider's mount pin when both the binding and its open handles are gone.
 * RU: Снять удержание подключения после закрытия привязки и открытых дескрипторов. */
static void destroy(void *state)
{
    DvmModuleRefs *service = state;
    styxclientfree(service->issuer);
    styxcallerrelease(service->local);
    free(service);
}


/* EN/RU: See module_refs.h. / См. module_refs.h. */
DvmModuleRefs *dvm_terminal_module_refs_install(DVM_Namespace *group)
{
    static const DVM_ProviderOps ops = {.info = info, .open = open_ref, .destroy = destroy, .access = access};
    DvmModuleRefs *service = calloc(1, sizeof(*service));
    if (!service || !dvm_terminal_entropy(service->scope, sizeof(service->scope)))
    {
        free(service);
        return NULL;
    }
    DVM_Provider *provider = dvm_provider_create(&ops, service);
    if (!provider)
    {
        free(service);
        return NULL;
    }
    DVM_NamespacePlan target;
    int mounted = dvm_namespace_plan(group, "/" STYXRPC_MODULE_REF_SCOPE_FILE, &target) &&
        dvm_namespace_mount_provider(group, &target, provider, "/styx-module-refs") == 1;
    dvm_provider_release(provider);
    return mounted ? service : NULL;
}


/* EN/RU: See module_refs.h. / См. module_refs.h. */
int dvm_terminal_module_refs_local(DvmModuleRefs *service, StyxCallerServer *local)
{
    if (!service || service->local || !styxcallerretain(local))
    {
        return 0;
    }
    service->local = local;
    return 1;
}


/* EN/RU: See module_refs.h. / См. module_refs.h. */
void dvm_terminal_module_refs_attach(DvmModuleRefs *service, struct StyxClient *issuer)
{
    if (service && !service->issuer && styxclientretain(issuer))
    {
        service->issuer = issuer;
    }
}
