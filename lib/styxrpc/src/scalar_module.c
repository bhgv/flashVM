/* Copyright (c) 2026 bhgv
 * EN: Expose one immutable scalar interface and independent function fids.
 * RU: Публикация неизменяемого скалярного интерфейса и независимых fid функций.
 */
#include "styxrpc/scalar_module.h"
#include <stdlib.h>
#include <string.h>

enum
{
    ROOT = 0,     /* Server expects logical root zero. */
    MOUNTED,      /* Directory containing published modules. */
    MODULE,       /* Loadable module directory. */
    INTERFACE,    /* Read-only compiled profile. */
    FIRST_EXPORT  /* Base ID for the registered function files. */
};

typedef struct ScalarFile
{
    const StyxRpcScalarModule *profile; /* Borrowed module for a profile read. */
    StyxRpcCall call;                  /* Private call for a function fid. */
} ScalarFile;


/* EN/RU: See scalar_module.h. / См. scalar_module.h. */
StyxRpcStatus styxrpc_scalar_module_init(StyxRpcScalarModule *module, const char *name,
    const StyxRpcExport *exports, size_t count)
{
    if (!module || !name || !*name || strlen(name) > STYXRPC_TREE_NAME ||
        strchr(name, '/') || !strcmp(name, ".") || !strcmp(name, "..") ||
        styxrpc_profile(exports, count, module->profile, sizeof(module->profile), &module->profile_size) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    module->exports = exports;
    module->count = count;
    module->entries[ROOT] = (StyxRpcEntry){ROOT, "/", 0555, 1};
    module->entries[MOUNTED] = (StyxRpcEntry){MOUNTED, "mounted", 0555, 1};
    module->entries[MODULE] = (StyxRpcEntry){MODULE, name, 0555, 1};
    module->entries[INTERFACE] = (StyxRpcEntry){INTERFACE, ".interface", 0444, 0};
    for (size_t i = 0; i < count; ++i)
    {
        module->entries[FIRST_EXPORT + i] = (StyxRpcEntry){FIRST_EXPORT + i, exports[i].name, 0777, 0};
    }
    return STYXRPC_OK;
}


/* EN: Return a borrowed immediate child by index, or NULL at end.
 * RU: Вернуть заимствованного непосредственного потомка или NULL в конце. */
static const StyxRpcEntry *enumerate(void *context, uint64_t dir, size_t index)
{
    const StyxRpcScalarModule *module = context;
    if ((dir == ROOT || dir == MOUNTED) && !index)
    {
        return module->entries + dir + 1;
    }
    return dir == MODULE && index <= module->count ? module->entries + INTERFACE + index : NULL;
}


/* EN: Copy metadata for the fixed identity id into node, or reject an unknown id.
 * RU: Скопировать метаданные устойчивого id в node или отклонить неизвестный id. */
static int describe(void *context, uint64_t id, StyxRpcNode *node)
{
    const StyxRpcScalarModule *module = context;
    if (id >= FIRST_EXPORT + module->count)
    {
        return 0;
    }
    *node = (StyxRpcNode){.entry = module->entries[id],
        .parent = id <= MOUNTED ? ROOT : id == MODULE ? MOUNTED : MODULE,
        .length = id == INTERFACE ? module->profile_size : 0};
    return 1;
}


/* EN: Allocate an independent cookie for a read-only profile or read/write call.
 * RU: Выделить независимое состояние для чтения профиля или вызова чтения/записи. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    const StyxRpcScalarModule *module = context;
    if (id < INTERFACE || id >= FIRST_EXPORT + module->count ||
        access != (id == INTERFACE ? STYXRPC_READ : STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    ScalarFile *file = calloc(1, sizeof(*file));
    if (!file)
    {
        return 0;
    }
    if (id == INTERFACE)
    {
        file->profile = module;
    }
    else if (styxrpc_open(&file->call, module->exports + (id - FIRST_EXPORT)) != STYXRPC_OK)
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
    const ScalarFile *file = cookie;
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
    return offset <= SIZE_MAX && styxrpc_read(&file->call, (size_t)offset, bytes, capacity, size) != STYXRPC_INVALID;
}


/* EN: Admit the next byte range and acknowledge it only when the call codec accepts it.
 * RU: Принять очередной диапазон байтов и подтвердить его после проверки кодеком. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    ScalarFile *file = cookie;
    if (file->profile || offset > SIZE_MAX ||
        styxrpc_write(&file->call, (size_t)offset, bytes, count) == STYXRPC_INVALID)
    {
        return 0;
    }
    *size = count;
    return 1;
}

const StyxRpcTree styxrpc_scalar_module_tree = {{enumerate, open_file, read_file, write_file, free}, describe}; /* Immutable tree operations. */
