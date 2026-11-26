/* Copyright (c) 2026 bhgv
 * EN: Adapt peer and local owner fids to the shared execute codec and mailbox.
 * RU: Подключить клиентские и локальные fid к общему кодеку исполнения и очереди.
 */
#include "exec_private.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct ExecCursor
{
    StyxRpcNodeService *codec; /* Owned codec cursor, retaining no managed data. */
    long offset; /* Position for the next ordinary VFS operation. */
} ExecCursor;


/* EN: Advertise a regular named protocol file at relative dot.
 * RU: Выдать метаданные именованного файла протокола в относительной точке. */
static FVM_Result info(const char *path, const char *name, unsigned mode, FVM_FileInfo *out)
{
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, name);
    out->mode = mode;
    out->identity.low = (uint32_t)STYXRPC_NODE_EXEC_QID;
    return FVM_OK;
}


/* EN: Give the peer file one stable name and identity for both walk and open.
 * RU: Выдать клиентскому файлу одно постоянное имя и идентификатор. */
static FVM_Result peer_info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    return info(path, STYXRPC_NODE_EXEC_SERVICE, 0666, out);
}


/* EN: Describe the exact open service cursor.
 * RU: Описать точный открытый курсор службы. */
static FVM_Result peer_cursor_info(void *state, FVM_FileInfo *out)
{
    return peer_info(state, ".", out);
}


/* EN: Read confirmed reply bytes through the private codec.
 * RU: Прочитать подтверждённые байты ответа через отдельный кодек. */
static FVM_Result peer_read(void *state, void *bytes, size_t size, size_t *used)
{
    ExecCursor *cursor = state;
    if (!styxrpc_node_service_read(cursor->codec, cursor->offset, bytes, size, used))
    {
        return FVM_ERROR;
    }
    cursor->offset += (long)*used;
    return FVM_OK;
}


/* EN: Stage an input fragment; only the final write invokes the local owner.
 * RU: Принять фрагмент входа. Только последняя запись вызывает владельца. */
static FVM_Result peer_write(void *state, const void *bytes, size_t size)
{
    ExecCursor *cursor = state;
    size_t used;
    if (!styxrpc_node_service_write(cursor->codec, cursor->offset, bytes, size, &used))
    {
        return FVM_ERROR;
    }
    cursor->offset += (long)used;
    return FVM_OK;
}


/* EN: Set one absolute byte position within the maximum framed request.
 * RU: Установить абсолютное смещение в пределах максимального запроса. */
static FVM_Result peer_seek(void *state, long offset, int origin, long *position)
{
    ExecCursor *cursor = state;
    if (origin != FVM_FILE_SEEK_START || offset < 0 ||
        offset > STYXRPC_NODE_BUFFER_MAX + STYXRPC_NODE_PATH_MAX + DVM_EXEC_HEADER + 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    cursor->offset = offset;
    *position = offset;
    return FVM_OK;
}


/* EN: Consume the private codec at clunk or carrier loss.
 * RU: Освободить отдельный кодек при clunk либо разрыве. */
static void peer_close(void *state)
{
    ExecCursor *cursor = state;
    styxrpc_node_service_close(cursor->codec);
    free(cursor);
}


/* EN: Open exactly one borrowed-broker read/write native peer cursor.
 * RU: Открыть точный read/write курсор, заимствующий брокер из провайдера. */
static FVM_Result peer_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps ops = {.info = peer_cursor_info, .read = peer_read,
        .write = peer_write, .seek = peer_seek, .close = peer_close};
    if (strcmp(path, ".") || (strcmp(mode, "rw") && strcmp(mode, "rwo")))
    {
        return FVM_UNSUPPORTED;
    }
    ExecCursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor || !(cursor->codec = styxrpc_node_service_open(dvm_exec_invoke, state)))
    {
        free(cursor);
        return FVM_OUT_OF_MEMORY;
    }
    *out = (DVM_VfsHandle){.ops = &ops, .state = cursor};
    return FVM_OK;
}


/* EN: The local owner file is reachable by the guest's own Sys, under its pinned group.
 * RU: Локальный файл владельца доступен через Sys в удерживаемой группе гостя. */
static FVM_Result owner_info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    return info(path, STYXRPC_NODE_EXEC_OWNER, 0600, out);
}


/* EN: Each owner open admits one publication and retains it until close.
 * RU: Каждое открытие принимает одну публикацию и удерживает её до закрытия. */
static FVM_Result owner_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps ops = {.read = dvm_exec_receive, .write = dvm_exec_answer,
        .close = dvm_exec_close_owner};
    if (strcmp(path, ".") || (strcmp(mode, "rw") && strcmp(mode, "rwo")))
    {
        return FVM_UNSUPPORTED;
    }
    ExecOwner *owner = calloc(1, sizeof(*owner));
    if (!owner)
    {
        return FVM_OUT_OF_MEMORY;
    }
    owner->broker = state;
    *out = (DVM_VfsHandle){.ops = &ops, .state = owner};
    return FVM_OK;
}


/* EN: Drop the exact launcher or provider reference. Destruction performs no group operation.
 * RU: Снять точную ссылку запуска или провайдера. Уничтожение не обращается к группе. */
void dvm_exec_release(void *state)
{
    DvmExecBroker *broker = state;
    pthread_mutex_lock(&broker->lock);
    int last = --broker->references == 0;
    pthread_mutex_unlock(&broker->lock);
    if (last)
    {
        pthread_cond_destroy(&broker->changed);
        pthread_mutex_destroy(&broker->lock);
        free(broker);
    }
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
void dvm_exec_retain(DvmExecBroker *broker)
{
    pthread_mutex_lock(&broker->lock);
    ++broker->references;
    pthread_mutex_unlock(&broker->lock);
}


/* EN: Return an owned provider pin with the given operations.
 * RU: Вернуть собственный провайдер с указанными операциями. */
static DVM_Provider *provider(DvmExecBroker *broker, const DVM_ProviderOps *ops)
{
    dvm_exec_retain(broker);
    DVM_Provider *result = dvm_provider_create(ops, broker);
    if (!result)
    {
        dvm_exec_release(broker);
    }
    return result;
}


/* EN: Create session-owned mailbox storage with an explicit monotonic clock.
 * RU: Создать хранилище очереди с монотонным таймером на время сессии. */
static DvmExecBroker *new_broker(DVM_Namespace *group)
{
    DvmExecBroker *broker = group ? calloc(1, sizeof(*broker)) : NULL;
    if (!broker || pthread_mutex_init(&broker->lock, NULL))
    {
        free(broker);
        return NULL;
    }
    pthread_condattr_t attr;
    int made = !pthread_condattr_init(&attr);
    int ready = made && !pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&broker->changed, &attr);
    if (made)
    {
        pthread_condattr_destroy(&attr);
    }
    if (!ready)
    {
        pthread_mutex_destroy(&broker->lock);
        free(broker);
        return NULL;
    }
    broker->references = 1;
    broker->group = group;
    return broker;
}


/* EN/RU: See executable_nodes.h. / См. executable_nodes.h. */
DvmExecBroker *dvm_exec_install(DVM_Namespace *group)
{
    static const DVM_ProviderOps ops = {.info = owner_info, .open = owner_open, .destroy = dvm_exec_release};
    DvmExecBroker *broker = new_broker(group);
    if (!broker)
    {
        return NULL;
    }
    DVM_Provider *local = provider(broker, &ops);
    DVM_NamespacePlan target;
    int ok = local && dvm_namespace_plan(group, "/" STYXRPC_NODE_EXEC_OWNER, &target) &&
        dvm_namespace_mount_provider(group, &target, local, "/styx-exec-owner") == 1;
    dvm_provider_release(local);
    if (!ok)
    {
        dvm_exec_release(broker);
        return NULL;
    }
    return broker;
}


/* EN/RU: See executable_nodes.h. / См. executable_nodes.h. */
DVM_Provider *dvm_exec_service(DvmExecBroker *broker)
{
    static const DVM_ProviderOps ops = {.info = peer_info, .open = peer_open, .destroy = dvm_exec_release};
    return broker ? provider(broker, &ops) : NULL;
}


/* EN/RU: See executable_nodes.h. / См. executable_nodes.h. */
void dvm_exec_stop(DvmExecBroker *broker)
{
    if (broker)
    {
        pthread_mutex_lock(&broker->lock);
        broker->stopped = 1;
        pthread_cond_broadcast(&broker->changed);
        pthread_mutex_unlock(&broker->lock);
        dvm_exec_release(broker);
    }
}
