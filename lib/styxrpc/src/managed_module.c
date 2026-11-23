/* Copyright (c) 2026 bhgv
 * EN: Expose one immutable managed interface and independent function fids.
 * RU: Публикация неизменяемого скалярного интерфейса и независимых fid функций.
 */
#include "styxrpc/managed_module.h"
#include "module_nodes.h"
#include <stdlib.h>
#include <string.h>

typedef struct ManagedFile
{
    const StyxRpcManagedModule *profile; /* Borrowed module for a profile read. */
    StyxRpcManagedCall call;                  /* Private call for a function fid. */
    unsigned char storage[]; /* Per-function request, reply and separate encoding scratch. */
} ManagedFile;


/* EN/RU: See managed_module.h. / См. managed_module.h. */
StyxRpcStatus styxrpc_managed_module_init(StyxRpcManagedModule *module, const char *name,
    const StyxRpcManagedExport *exports, size_t count)
{
    if (!module || !styxrpc_module_name(name) ||
        !exports || !count || count > STYXRPC_MAX_EXPORTS)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcManagedModule prepared;
    prepared.profile_size = 0;
    for (size_t i = 0; i < count; ++i)
    {
        size_t written;
        if (styxrpc_managed_profile(exports + i, prepared.profile + prepared.profile_size,
            sizeof(prepared.profile) - prepared.profile_size, &written) != STYXRPC_OK)
        {
            return STYXRPC_INVALID;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (!strcmp(exports[i].name, exports[j].name))
            {
                return STYXRPC_INVALID;
            }
        }
        prepared.profile_size += written;
    }
    memcpy(module->profile, prepared.profile, prepared.profile_size);
    module->profile_size = prepared.profile_size;
    module->exports = exports;
    module->count = count;
    styxrpc_module_nodes(module->entries, name);
    for (size_t i = 0; i < count; ++i)
    {
        styxrpc_module_export_node(module->entries, i, exports[i].name);
    }
    return STYXRPC_OK;
}


/* EN: Return a borrowed immediate child by index, or NULL at end.
 * RU: Вернуть заимствованного непосредственного потомка или NULL в конце. */
static const StyxRpcEntry *enumerate(void *context, uint64_t dir, size_t index)
{
    const StyxRpcManagedModule *module = context;
    return styxrpc_module_child(module->entries, module->count, dir, index);
}


/* EN: Copy metadata for the fixed identity id into node, or reject an unknown id.
 * RU: Скопировать метаданные устойчивого id в node или отклонить неизвестный id. */
static int describe(void *context, uint64_t id, StyxRpcNode *node)
{
    const StyxRpcManagedModule *module = context;
    return styxrpc_module_stat(module->entries, module->count, module->profile_size, id, node);
}


/* EN: Allocate an independent cookie for a read-only profile or read/write call.
 * RU: Выделить независимое состояние для чтения профиля или вызова чтения/записи. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    const StyxRpcManagedModule *module = context;
    if (id < INTERFACE || id >= FIRST_EXPORT + module->count ||
        access != (id == INTERFACE ? STYXRPC_READ : STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    size_t extent = id == INTERFACE ? 0 : 3U * STYXRPC_MANAGED_LIMIT;
    ManagedFile *file = calloc(1, sizeof(*file) + extent);
    if (!file)
    {
        return 0;
    }
    if (id == INTERFACE)
    {
        file->profile = module;
    }
    else if (styxrpc_managed_open(&file->call, module->exports + (id - FIRST_EXPORT),
        file->storage, STYXRPC_MANAGED_LIMIT, file->storage + STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT) != STYXRPC_OK)
    {
        free(file);
        return 0;
    }
    *cookie = file;
    return 1;
}


/* EN: Read a checked slice from the borrowed cookie; zero bytes at EOF or while pending.
 * RU: Прочитать проверенный фрагмент из cookie; ноль байт в конце или при ожидании. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    const ManagedFile *file = cookie;
    if (file->profile)
    {
        size_t remaining = offset < file->profile->profile_size ? file->profile->profile_size - (size_t)offset : 0;
        *size = remaining < capacity ? remaining : capacity;
        if (*size)
        {
            memcpy(bytes, file->profile->profile + offset, *size);
        }
        return 1;
    }
    return offset <= SIZE_MAX && styxrpc_managed_read(&file->call, (size_t)offset, bytes, capacity, size) != STYXRPC_INVALID;
}


/* EN: Admit the next byte range and acknowledge it only when the call codec accepts it.
 * RU: Принять очередной диапазон байтов и подтвердить его после проверки кодеком. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    ManagedFile *file = cookie;
    if (file->profile || offset > SIZE_MAX)
    {
        return 0;
    }
    unsigned char *scratch = file->storage + 2U * STYXRPC_MANAGED_LIMIT;
    StyxRpcStatus status = styxrpc_managed_write(&file->call, (size_t)offset, bytes, count,
        scratch, STYXRPC_MANAGED_LIMIT);
    if (status == STYXRPC_MORE && file->call.pending)
    {
        /* EN: This adapter has no asynchronous completion owner.
         * RU: У этого адаптера нет владельца асинхронного завершения. */
        styxrpc_managed_close(&file->call);
        status = STYXRPC_INVALID;
    }
    if (status == STYXRPC_INVALID)
    {
        return 0;
    }
    *size = count;
    return 1;
}

/* EN: Consume the open managed fid, cancelling any outstanding application work.
 * RU: Освободить открытый fid, отменив незавершённую операцию приложения. */
static void close_file(void *cookie)
{
    ManagedFile *file = cookie;
    styxrpc_managed_close(&file->call);
    free(file);
}

const StyxRpcTree styxrpc_managed_module_tree = {{enumerate, open_file, read_file, write_file, close_file}, describe}; /* Immutable tree operations. */
