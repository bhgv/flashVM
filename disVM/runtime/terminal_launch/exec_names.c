/* Copyright (c) 2026 bhgv
 * EN: Reserve callback-backed VFS nodes without replacing existing namespace entries.
 * RU: Резервировать узлы VFS с колбэками без замены существующих записей.
 */
#include "exec_private.h"
#include "styxrpc/named_channel.h"
#include <stdlib.h>
#include <string.h>

#define EXEC_MARKER_NAMESPACE UINT32_C(0x40000000) /* High word of a callback node's virtual Qid. */

typedef struct ExecMarker
{
    DvmExecBroker *broker; /* Owned while the namespace, walk or open pins this marker. */
    unsigned mode; /* Immutable FVM_ACCESS bits permitted on this node. */
    uint32_t serial; /* Stable session-local Qid low word. */
    char name[]; /* Owned displayed basename. */
} ExecMarker;


/* EN: Describe the node with its fixed publication permissions.
 * RU: Описать узел с правами, заданными при публикации. */
static FVM_Result marker_info(void *state, const char *path, FVM_FileInfo *out)
{
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    const ExecMarker *marker = state;
    memset(out, 0, sizeof(*out));
    strcpy(out->name, marker->name);
    out->mode = marker->mode * 0111U;
    out->identity.high = EXEC_MARKER_NAMESPACE;
    out->identity.low = marker->serial;
    return FVM_OK;
}


/* EN: Grant only requested capabilities of a still-active callback node.
 * RU: Разрешить только запрошенные операции ещё действующего узла с колбэком. */
static FVM_Result marker_access(void *state, const char *path, unsigned access)
{
    const ExecMarker *marker = state;
    if (strcmp(path, ".") || !dvm_exec_active(marker->broker, marker->serial))
    {
        return FVM_NOT_FOUND;
    }
    return (access & marker->mode) == access ? FVM_OK : FVM_UNSUPPORTED;
}


/* EN: Describe the opened node retained by its provider.
 * RU: Описать открытый узел, удерживаемый провайдером. */
static FVM_Result marker_open_info(void *state, FVM_FileInfo *out)
{
    return marker_info(state, ".", out);
}


/* EN: The provider wrapper releases its pin after this borrowed cursor closes.
 * RU: Обёртка провайдера снимает ссылку после закрытия заимствованного курсора. */
static void marker_close(void *state)
{
    (void)state;
}


/* EN: Move one ordinary data chunk through the exact owner and report its confirmed prefix.
 * RU: Передать один чанк данных точному владельцу и вернуть подтверждённый префикс. */
static void transfer(ExecMarker *marker, NodeOperation operation, const void *input, void *output,
    size_t size, DVM_IoResult *result)
{
    size_t capacity = size > STYXRPC_NODE_BUFFER_MAX ? STYXRPC_NODE_BUFFER_MAX : size;
    size_t used = 0;
    NodeOutcome outcome = dvm_exec_transfer(marker->broker, marker->serial, operation, input, output, capacity, &used);
    *result = (DVM_IoResult){.status = FVM_OK, .count = used, .flags = DVM_IO_PREFIX};
    if (outcome == DVM_NODE_UNCONFIRMED)
    {
        result->status = FVM_ERROR;
        result->stop = DVM_IO_FAILURE;
        result->flags |= DVM_IO_UNCERTAIN;
    }
    else if (outcome == DVM_NODE_REFUSED)
    {
        result->status = FVM_ERROR;
        result->stop = DVM_IO_DENIED;
    }
    else if (operation == DVM_NODE_READ && !used)
    {
        result->stop = DVM_IO_EOF;
    }
}


/* EN: Fill the caller's read buffer. The callback reports the prefix or EOF.
 * RU: Заполнить буфер чтения вызывающей стороны. Колбэк сообщает префикс или EOF. */
static void marker_read(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    transfer(state, DVM_NODE_READ, NULL, bytes, size, result);
}


/* EN: Send borrowed input to the owner and return only the prefix it acknowledges.
 * RU: Передать заимствованный вход владельцу и вернуть только подтверждённый префикс. */
static void marker_write(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    transfer(state, DVM_NODE_WRITE, bytes, NULL, size, result);
}


/* EN: Return a sequential file handle with operations limited to the granted open mode.
 * RU: Вернуть последовательный файл, ограниченный операциями указанного режима открытия. */
static FVM_Result marker_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps read_ops = {.info = marker_open_info, .read_chunk = marker_read,
        .close = marker_close};
    static const DVM_VfsOps write_ops = {.info = marker_open_info, .write_chunk = marker_write,
        .close = marker_close};
    static const DVM_VfsOps both_ops = {.info = marker_open_info, .read_chunk = marker_read,
        .write_chunk = marker_write, .close = marker_close};
    unsigned access;
    const DVM_VfsOps *ops;
    if (!strcmp(mode, "r"))
    {
        ops = &read_ops;
        access = FVM_ACCESS_READ;
    }
    else if (!strcmp(mode, "wo"))
    {
        ops = &write_ops;
        access = FVM_ACCESS_WRITE;
    }
    else if (!strcmp(mode, "rw") || !strcmp(mode, "rwo"))
    {
        ops = &both_ops;
        access = FVM_ACCESS_READ | FVM_ACCESS_WRITE;
    }
    else
    {
        return FVM_UNSUPPORTED;
    }
    FVM_Result status = marker_access(state, path, access);
    if (status == FVM_OK)
    {
        *out = (DVM_VfsHandle){.ops = ops, .state = state};
    }
    return status;
}


/* EN: Release the broker pin after the last provider handle closes.
 * RU: Освободить ссылку на брокер после закрытия последнего дескриптора провайдера. */
static void marker_destroy(void *state)
{
    ExecMarker *marker = state;
    dvm_exec_release(marker->broker);
    free(marker);
}


/* EN: Check traverse and write rights on the current parent before publication.
 * RU: Перед публикацией проверить права прохода и записи в текущий родительский каталог. */
static int parent_writable(DVM_Namespace *group, const char *path)
{
    char parent[STYXRPC_NODE_PATH_MAX + 2];
    size_t length = (size_t)(strrchr(path, '/') - path);
    if (!length)
    {
        length = 1;
    }
    memcpy(parent, path, length);
    parent[length] = 0;
    DVM_ResourcePlan plan;
    if (!dvm_resource_plan(group, parent, 0, &plan))
    {
        return 0;
    }
    unsigned access = FVM_ACCESS_WRITE | FVM_ACCESS_EXEC;
    plan.requested_access = access;
    FVM_Result status = dvm_union_select(&plan, 0);
    if (status == FVM_OK && plan.directory)
    {
        status = plan.provider ? dvm_provider_access(plan.provider, plan.relative, access) :
            plan.anchor ? fvm_port_anchor_access_at(dvm_anchor_port(plan.anchor), plan.relative, access) : FVM_UNSUPPORTED;
    }
    else
    {
        status = FVM_UNSUPPORTED;
    }
    dvm_resource_close(&plan);
    return status == FVM_OK;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
int dvm_exec_claim(ExecOwner *owner, const char *absolute)
{
    static const DVM_ProviderOps ops = {.info = marker_info, .access = marker_access,
        .open = marker_open, .destroy = marker_destroy};
    if (!styxrpc_named_path(absolute))
    {
        return 0;
    }
    const char *basename = strrchr(absolute, '/') + 1;
    size_t length = strlen(basename);
    if (length >= sizeof(((FVM_FileInfo *)0)->name))
    {
        return 0;
    }
    DVM_Namespace *group = owner->broker->group;
    DVM_NamespacePlan target;
    DVM_ResourcePlan plan;
    if (!dvm_namespace_plan(group, absolute, &target) || !parent_writable(group, absolute) ||
        !dvm_resource_plan(group, absolute, 0, &plan))
    {
        return 0;
    }
    FVM_FileInfo info;
    plan.absence_only = 1;
    FVM_Result status = dvm_resource_info(&plan, &info);
    dvm_resource_close(&plan);
    if (status != FVM_NOT_FOUND)
    {
        return 0;
    }
    ExecMarker *entry = malloc(sizeof(*entry) + length + 1);
    if (!entry)
    {
        return 0;
    }
    entry->broker = owner->broker;
    entry->mode = owner->mode;
    strcpy(entry->name, basename);
    dvm_exec_retain(entry->broker);
    pthread_mutex_lock(&owner->broker->lock);
    entry->serial = owner->broker->stopped || owner->broker->marker == UINT32_MAX ? 0 : ++owner->broker->marker;
    pthread_mutex_unlock(&owner->broker->lock);
    DVM_Provider *marker = entry->serial ? dvm_provider_create(&ops, entry) : NULL;
    if (!marker)
    {
        marker_destroy(entry);
        return 0;
    }
    owner->marker = entry->serial;
    int mounted = dvm_namespace_mount_provider_id(group, &target, marker, "/styx-executable", &owner->layer) == 1;
    dvm_provider_release(marker);
    if (mounted)
    {
        strcpy(owner->path, absolute);
    }
    return mounted;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
uint64_t dvm_exec_layer(DvmExecBroker *broker, const char *absolute)
{
    DVM_ResourcePlan plan;
    if (!dvm_resource_plan(broker->group, absolute, 0, &plan))
    {
        return 0;
    }
    FVM_FileInfo info;
    uint64_t id = dvm_resource_info(&plan, &info) == FVM_OK && !info.directory && (info.mode & 0111) ?
        plan.layer_id : 0;
    dvm_resource_close(&plan);
    return id;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
void dvm_exec_unclaim(ExecOwner *owner)
{
    DVM_Namespace *group = owner->broker->group;
    DVM_NamespacePlan target;
    while (dvm_namespace_plan(group, owner->path, &target) &&
        !dvm_namespace_unmount_layer(group, &target, owner->layer))
    {
    }
}
