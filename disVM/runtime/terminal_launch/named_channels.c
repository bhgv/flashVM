/* Copyright (c) 2026 bhgv
 * EN: Serve the named-channel protocol on one private DisVM VFS node.
 * RU: Обслуживать протокол имён каналов на закрытом узле VFS DisVM.
 */
#include "named_channels.h"
#include "styxrpc/named_cell.h"
#include "entropy.h"
#include <stdlib.h>
#include <string.h>

typedef struct NamedCursor
{
    StyxRpcNamedEndpoint *endpoint; /* Owned registration and wire framing state. */
    long position;                  /* Exact current offset on this descriptor. */
} NamedCursor;


/* EN: Describe the private service file. Relative names other than root are absent.
 * RU: Описать закрытый служебный файл. Другие относительные имена отсутствуют. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    strcpy(output->name, STYXRPC_NAMED_SERVICE);
    output->mode = 0666;
    return FVM_OK;
}


/* EN: Authorize read/write on the private control node.
 * RU: Разрешить чтение и запись служебного узла. */
static FVM_Result access(void *state, const char *path, unsigned requested)
{
    (void)state;
    return !strcmp(path, ".") && !(requested & FVM_ACCESS_EXEC) ? FVM_OK : FVM_NOT_FOUND;
}


/* EN: Describe an open control fid as the same immutable service file.
 * RU: Описать открытый управляющий fid как тот же постоянный служебный файл. */
static FVM_Result cursor_info(void *state, FVM_FileInfo *output)
{
    return info(state, ".", output);
}


/* EN: Read a reply fragment; a short zero while pending leaves the position intact.
 * RU: Прочитать фрагмент ответа. Ноль во время ожидания сохраняет позицию. */
static FVM_Result cursor_read(void *state, void *bytes, size_t size, size_t *used)
{
    NamedCursor *cursor = state;
    if (!styxrpc_named_reply(cursor->endpoint, cursor->position, bytes, size, used))
    {
        return FVM_ERROR;
    }
    cursor->position += (long)*used;
    return FVM_OK;
}


/* EN: Stage bytes at this offset or reject without acknowledging the fragment.
 * RU: Подготовить байты по смещению либо отклонить фрагмент без подтверждения. */
static FVM_Result cursor_write(void *state, const void *bytes, size_t size)
{
    NamedCursor *cursor = state;
    if (!styxrpc_named_submit(cursor->endpoint, cursor->position, bytes, size))
    {
        return FVM_ERROR;
    }
    cursor->position += (long)size;
    return FVM_OK;
}


/* EN: Select an absolute protocol position for one command or reply.
 * RU: Выбрать абсолютное смещение в текущей команде или ответе. */
static FVM_Result cursor_seek(void *state, long offset, int origin, long *out)
{
    NamedCursor *cursor = state;
    if (origin != FVM_FILE_SEEK_START || offset < 0 || offset > STYXRPC_NAMED_MAX + STYXRPC_NAMED_HEADER)
    {
        return FVM_INVALID_ARGUMENT;
    }
    cursor->position = offset;
    *out = offset;
    return FVM_OK;
}


/* EN: Drop one endpoint and retire its reservation if it is the last participant.
 * RU: Закрыть участника и снять резерв имени, если других участников нет. */
static void cursor_close(void *state)
{
    NamedCursor *cursor = state;
    styxrpc_named_retire(cursor->endpoint);
    free(cursor);
}


/* EN: Transfer a fresh private control cursor when opened for both directions.
 * RU: Передать новый частный управляющий курсор при открытии в обе стороны. */
static FVM_Result open_control(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps ops = {.info = cursor_info, .read = cursor_read,
        .write = cursor_write, .seek = cursor_seek, .close = cursor_close};
    if (strcmp(path, ".") || (strcmp(mode, "rwo") && strcmp(mode, "rw")))
    {
        return FVM_UNSUPPORTED;
    }
    NamedCursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor || !(cursor->endpoint = styxrpc_named_endpoint(state)))
    {
        free(cursor);
        return FVM_OUT_OF_MEMORY;
    }
    *out = (DVM_VfsHandle){.ops = &ops, .state = cursor};
    return FVM_OK;
}


/* EN: Read current names through the guest's namespace without changing its cwd.
 * RU: Проверить текущие имена в пространстве гостя независимо от его cwd. */
static FVM_Result lookup(DVM_Namespace *group, const char *path, unsigned requested, FVM_FileInfo *output)
{
    DVM_ResourcePlan plan;
    if (!dvm_resource_plan(group, path, 0, &plan))
    {
        return FVM_ERROR;
    }
    plan.absence_only = 1;
    FVM_Result status = dvm_resource_info(&plan, output);
    if (status == FVM_OK && requested)
    {
        if (plan.provider)
        {
            status = dvm_provider_access(plan.provider, plan.relative, requested);
        }
        else
        {
            status = plan.anchor ? fvm_port_anchor_access_at(dvm_anchor_port(plan.anchor), plan.relative,
                requested) : FVM_UNSUPPORTED;
        }
    }
    dvm_resource_close(&plan);
    return status;
}


/* EN: Deny ordinary file I/O to a name reserved for rendezvous.
 * RU: Запретить обычный файловый доступ по зарезервированному имени. */
static FVM_Result refuse_file(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)out;
    return FVM_UNSUPPORTED;
}


/* EN: A reserved name has service metadata, with no ordinary permissions.
 * RU: У зарезервированного имени служебные метаданные без обычных прав. */
static FVM_Result marker_info(void *state, const char *path, FVM_FileInfo *out)
{
    FVM_Result result = info(state, path, out);
    if (result == FVM_OK)
    {
        out->mode = 0;
    }
    return result;
}


/* EN: No state belongs to a name marker.
 * RU: У метки имени нет собственного состояния. */
static void drop_marker(void *state)
{
    (void)state;
}


/* EN: Atomically reserve a missing VFS name at the checked namespace revision.
 * RU: Зарезервировать свободное имя на проверенной ревизии VFS. */
static uint64_t claim(void *state, const char *path)
{
    static const DVM_ProviderOps marker_ops = {.info = marker_info, .open = refuse_file,
        .destroy = drop_marker};
    DVM_Namespace *group = state;
    char parent[STYXRPC_VFS_PATH_MAX];
    size_t length = (size_t)(strrchr(path, '/') - path);
    if (!length)
    {
        length = 1;
    }
    memcpy(parent, path, length);
    parent[length] = 0;
    DVM_NamespacePlan target;
    FVM_FileInfo entry;
    if (!dvm_namespace_plan(group, path, &target) ||
        lookup(group, parent, FVM_ACCESS_WRITE | FVM_ACCESS_EXEC, &entry) != FVM_OK || !entry.directory ||
        lookup(group, path, 0, &entry) != FVM_NOT_FOUND)
    {
        return 0;
    }
    DVM_Provider *marker = dvm_provider_create(&marker_ops, NULL);
    uint64_t id = 0;
    if (marker)
    {
        dvm_namespace_mount_provider_id(group, &target, marker, "/styx-rpc-channel", &id);
        dvm_provider_release(marker);
    }
    return id;
}


/* EN: Reuse the channel only while its exact layer is the current VFS selection.
 * RU: Использовать канал, пока его точный слой остаётся выбранным в VFS. */
static int visible(void *state, const char *path, uint64_t id)
{
    DVM_ResourcePlan plan;
    if (!dvm_resource_plan(state, path, 0, &plan))
    {
        return 0;
    }
    int same = plan.layer_id == id;
    dvm_resource_close(&plan);
    return same;
}


/* EN: Remove only this channel's exact binding, retrying a stale namespace snapshot.
 * RU: Снять только точную привязку канала, повторяя проверку устаревшей ревизии. */
static void unclaim(void *state, const char *path, uint64_t id)
{
    DVM_NamespacePlan target;
    DVM_Namespace *group = state;
    while (dvm_namespace_plan(group, path, &target) && !dvm_namespace_unmount_layer(group, &target, id))
    {
    }
}


/* EN: Dispose after the namespace and its retained cursors release the provider.
 * RU: Освободить после снятия ссылок пространства имён и открытых курсоров. */
static void destroy(void *state)
{
    styxrpc_named_destroy(state);
}


/* EN: Fill the issuing session's capability nonce using host entropy.
 * RU: Заполнить nonce полномочия выдавшей сессии системной энтропией. */
static int entropy(void *state, unsigned char *bytes, size_t size)
{
    (void)state;
    return dvm_terminal_entropy(bytes, size);
}


/* EN/RU: See named_channels.h. / См. named_channels.h. */
int dvm_named_channels_install(DVM_Namespace *group)
{
    static const DVM_ProviderOps ops = {.info = info, .open = open_control, .destroy = destroy, .access = access};
    StyxRpcNamedRegistry *registry = styxrpc_named_registry(claim, visible, unclaim, group);
    DVM_Provider *provider = styxrpc_named_cells_enable(registry, entropy, NULL) ? dvm_provider_create(&ops, registry) : NULL;
    if (!provider)
    {
        styxrpc_named_destroy(registry);
        return 0;
    }
    DVM_NamespacePlan target;
    int mounted = dvm_namespace_plan(group, "/" STYXRPC_NAMED_SERVICE, &target) &&
        dvm_namespace_mount_provider(group, &target, provider, "/styx-rpc-channels") == 1;
    dvm_provider_release(provider);
    return mounted;
}
