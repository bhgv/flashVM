/* Copyright (c) 2026 bhgv
 * EN: Reserve callback-backed VFS nodes without replacing existing namespace entries.
 * RU: Резервировать узлы VFS с колбэками без замены существующих записей.
 */
#include "exec_private.h"
#include "styxrpc/module_ref.h"
#include "styxrpc/named_channel.h"
#include "styxrpc/variable_file.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define EXEC_MARKER_NAMESPACE UINT32_C(0x40000000) /* High word of a callback node's virtual Qid. */

typedef struct ExecMarker
{
    DvmExecBroker *broker; /* Owned while the namespace, walk or open pins this marker. */
    unsigned mode; /* Immutable FVM_ACCESS bits permitted on this node. */
    uint64_t length; /* Fixed byte extent in positioned mode. */
    int positioned; /* Whether file handles accept random-access I/O. */
    uint32_t serial; /* Stable session-local Qid low word. */
    char name[]; /* Owned displayed basename. */
} ExecMarker;


/* EN: State of one ordinary open, serialized by its fid and retaining the provider.
 * RU: Состояние одного открытия. Fid сериализует доступ и удерживает провайдера. */
typedef struct NodeCursor
{
    ExecMarker *marker; /* Borrowed from the provider wrapper that owns this cursor. */
    uint64_t offset; /* Next byte position in positioned mode. */
    unsigned access; /* FVM_ACCESS mask fixed at open. */
    int uncertain; /* An unacknowledged operation prevents further I/O on this cursor. */
} NodeCursor;


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
    out->length.high = (uint32_t)(marker->length >> 32);
    out->length.low = (uint32_t)marker->length;
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
    NodeCursor *cursor = state;
    return marker_info(cursor->marker, ".", out);
}


/* EN: Release this cursor. The provider wrapper retains the marker through close.
 * RU: Освободить курсор. Обёртка провайдера удерживает метку до завершения close. */
static void marker_close(void *state)
{
    free(state);
}


/* EN: Return the bounded data window. Sequential nodes keep the full offered size.
 * RU: Вернуть доступный интервал. Последовательный узел получает весь предложенный размер. */
static size_t available(const NodeCursor *cursor, size_t size)
{
    if (cursor->marker->positioned)
    {
        uint64_t length = cursor->marker->length;
        if (cursor->offset >= length)
        {
            return 0;
        }
        uint64_t remaining = length - cursor->offset;
        if (remaining < size)
        {
            return (size_t)remaining;
        }
    }
    return size;
}


/* EN: Move one permitted data chunk and advance only by the confirmed prefix.
 * RU: Передать разрешённый фрагмент и продвинуть курсор на подтверждённый префикс. */
static void transfer(NodeCursor *cursor, NodeOperation operation, unsigned access, const void *input,
    void *output, size_t size, DVM_IoResult *result)
{
    *result = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (!(cursor->access & access) || cursor->uncertain)
    {
        return;
    }
    ExecMarker *marker = cursor->marker;
    size_t window = available(cursor, size);
    if (operation == DVM_NODE_WRITE && window < size)
    {
        return;
    }
    size_t capacity = window > STYXRPC_NODE_BUFFER_MAX ? STYXRPC_NODE_BUFFER_MAX : window;
    size_t used = 0;
    NodeOutcome outcome;
    if (marker->positioned && !capacity)
    {
        outcome = dvm_exec_active(marker->broker, marker->serial) ? DVM_NODE_ACCEPTED : DVM_NODE_REFUSED;
    }
    else
    {
        outcome = dvm_exec_transfer(marker->broker, marker->serial, operation, cursor->offset,
            input, output, capacity, &used);
    }
    if (outcome == DVM_NODE_UNCONFIRMED)
    {
        cursor->uncertain = 1;
        result->stop = DVM_IO_FAILURE;
        result->flags |= DVM_IO_UNCERTAIN;
    }
    else if (outcome == DVM_NODE_ACCEPTED)
    {
        if (marker->positioned)
        {
            cursor->offset += used;
        }
        result->status = FVM_OK;
        result->count = used;
        result->stop = operation == DVM_NODE_READ && !used ? DVM_IO_EOF : DVM_IO_NONE;
    }
}


/* EN: Fill the caller's read buffer. The callback reports the prefix or EOF.
 * RU: Заполнить буфер чтения вызывающей стороны. Колбэк сообщает префикс или EOF. */
static void marker_read(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    transfer(state, DVM_NODE_READ, FVM_ACCESS_READ, NULL, bytes, size, result);
}


/* EN: Send borrowed input to the owner and return only the prefix it acknowledges.
 * RU: Передать заимствованный вход владельцу и вернуть только подтверждённый префикс. */
static void marker_write(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    transfer(state, DVM_NODE_WRITE, FVM_ACCESS_WRITE, bytes, NULL, size, result);
}


/* EN: Set this cursor relative to start, current offset or declared EOF, without calling Limbo.
 * RU: Изменить смещение от начала, текущей позиции или объявленного EOF без вызова Limbo. */
static FVM_Result marker_seek(void *state, long offset, int origin, long *position)
{
    NodeCursor *cursor = state;
    if (origin < FVM_FILE_SEEK_START || origin > FVM_FILE_SEEK_END || cursor->uncertain ||
        !dvm_exec_active(cursor->marker->broker, cursor->marker->serial))
    {
        return FVM_ERROR;
    }
    uint64_t base = 0;
    if (origin == FVM_FILE_SEEK_CURRENT)
    {
        base = cursor->offset;
    }
    else if (origin == FVM_FILE_SEEK_END)
    {
        base = cursor->marker->length;
    }
    uint64_t magnitude = offset < 0 ? (uint64_t)(-(offset + 1)) + 1 : (uint64_t)offset;
    uint64_t limit = (uint64_t)LONG_MAX < STYXRPC_NODE_OFFSET_MAX ? (uint64_t)LONG_MAX : STYXRPC_NODE_OFFSET_MAX;
    if (base > limit || (offset < 0 ? magnitude > base : magnitude > limit - base))
    {
        return FVM_INVALID_ARGUMENT;
    }
    cursor->offset = offset < 0 ? base - magnitude : base + magnitude;
    *position = (long)cursor->offset;
    return FVM_OK;
}


/* EN: Open an independent cursor with operations limited to the granted mode.
 * RU: Открыть независимый курсор с операциями в пределах разрешённого режима. */
static FVM_Result marker_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps stream = {.info = marker_open_info, .read_chunk = marker_read,
        .write_chunk = marker_write, .close = marker_close};
    static const DVM_VfsOps positioned = {.info = marker_open_info, .read_chunk = marker_read,
        .write_chunk = marker_write, .close = marker_close, .seek = marker_seek};
    unsigned access;
    if (!strcmp(mode, "r"))
    {
        access = FVM_ACCESS_READ;
    }
    else if (!strcmp(mode, "wo"))
    {
        access = FVM_ACCESS_WRITE;
    }
    else if (!strcmp(mode, "rw") || !strcmp(mode, "rwo"))
    {
        access = FVM_ACCESS_READ | FVM_ACCESS_WRITE;
    }
    else
    {
        return FVM_UNSUPPORTED;
    }
    FVM_Result status = marker_access(state, path, access);
    if (status != FVM_OK)
    {
        return status;
    }
    NodeCursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    cursor->marker = state;
    cursor->access = access;
    *out = (DVM_VfsHandle){.ops = cursor->marker->positioned ? &positioned : &stream, .state = cursor};
    return FVM_OK;
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


/* EN: Admit the reserved .var component only for a typed positioned export.
 * RU: Допустить служебный компонент .var только для типизированного позиционного экспорта. */
static int module_variable(const ExecOwner *owner, const char *absolute)
{
    if (absolute[0] != '/' || !owner->positioned || owner->length != STYX_VARIABLE_INT_SIZE ||
        !(owner->mode & FVM_ACCESS_READ) || (owner->mode & FVM_ACCESS_EXEC) ||
        !styxrpc_module_ref_path(absolute + 1))
    {
        return 0;
    }
    const char *image = absolute + sizeof("/mounted/") - 1;
    const char *member = strrchr(absolute, '/') + 1;
    return !styxrpc_vfs_service_name(image, strcspn(image, "/")) &&
        !styxrpc_vfs_service_name(member, strlen(member));
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
int dvm_exec_claim(ExecOwner *owner, const char *absolute)
{
    static const DVM_ProviderOps ops = {.info = marker_info, .access = marker_access,
        .open = marker_open, .destroy = marker_destroy};
    if (!styxrpc_named_path(absolute) && !module_variable(owner, absolute))
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
    entry->positioned = owner->positioned;
    entry->length = owner->length;
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
