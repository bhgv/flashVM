/* Copyright (c) 2026 bhgv
 * EN: Publish user callback files and implicit directories within one live image.
 * RU: Публикация пользовательских файлов и неявных каталогов одного живого образа.
 */
#include "nodes_internal.h"
#include <stdlib.h>
#include <string.h>

typedef struct CallbackNode
{
    StyxRpcNode node;              /* Copied stable identity and metadata. */
    StyxRpcNodeCallbacks callback; /* Adopted file state, empty for directories. */
    char name[STYXRPC_TREE_NAME + 1]; /* Component owned by this record. */
} CallbackNode;

struct StyxRpcNodes
{
    CallbackNode entries[STYXRPC_NODE_LIMIT]; /* Stable file and directory storage. */
    size_t count;                            /* Published prefix. */
};

/* EN: Resolve one ordinary absolute-from-attach component, rejecting service aliases.
 * RU: Проверить компонент от корня attach и исключить служебные имена. */
static int component(const char *name, size_t length, int root)
{
    static const char *const reserved[] = {".", "..", ".fd", ".prog", ".pid", ".var", ".vfs",
        ".unibus", ".rpc", ".messages", ".interface", ".programs", ".sysevents", ".state",
        ".upload", ".exit", ".idx", ".val", ".len", ".type", ".ins", ".push", ".pop",
        ".add", ".count"};
    if (!length || length > STYXRPC_TREE_NAME || (root && length == 7 && !memcmp(name, "mounted", 7)))
    {
        return 0;
    }
    for (size_t i = 0; i < sizeof(reserved) / sizeof(*reserved); ++i)
    {
        if (strlen(reserved[i]) == length && !memcmp(name, reserved[i], length))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Locate a child in the published prefix. Temporary additions follow a new parent.
 * RU: Найти потомка в опубликованном префиксе. Временные узлы идут под новым родителем. */
static CallbackNode *child(StyxRpcNodes *nodes, uint64_t parent, const char *name, size_t length)
{
    for (size_t i = 0; i < nodes->count; ++i)
    {
        CallbackNode *entry = nodes->entries + i;
        if (entry->node.parent == parent && strlen(entry->name) == length &&
            !memcmp(entry->name, name, length))
        {
            return entry;
        }
    }
    return NULL;
}


/* EN: Stage one complete path into the unused prefix; never expose partial directories.
 * RU: Подготовить полный путь вне опубликованного префикса, без частичных каталогов. */
static int stage(StyxRpcNodes *nodes, uint64_t *next, const char *path, unsigned mode,
    const StyxRpcNodeCallbacks *callbacks)
{
    size_t count = nodes->count;
    uint64_t parent = 0;
    uint64_t id = *next;
    for (;;)
    {
        size_t length = strcspn(path, "/");
        int directory = path[length] == '/';
        if (!component(path, length, parent == 0))
        {
            return 0;
        }
        CallbackNode *entry = child(nodes, parent, path, length);
        if (entry)
        {
            if (!directory || !entry->node.entry.directory)
            {
                return 0;
            }
        }
        else
        {
            if (count == STYXRPC_NODE_LIMIT || id > UINT32_MAX)
            {
                return 0;
            }
            entry = nodes->entries + count++;
            memset(entry, 0, sizeof(*entry));
            memcpy(entry->name, path, length);
            entry->node.entry = (StyxRpcEntry){id++, entry->name, directory ? 0555U : mode * 0111U, directory};
            entry->node.parent = parent;
        }
        if (!directory)
        {
            entry->callback = *callbacks;
            nodes->count = count;
            *next = id;
            return 1;
        }
        parent = entry->node.entry.id;
        path += length + 1;
    }
}


/* EN/RU: See nodes_internal.h. / См. nodes_internal.h. */
StyxRpcStatus styxrpc_nodes_insert(StyxRpcNodes **nodes, uint64_t *next, const char *path,
    unsigned mode, const StyxRpcNodeCallbacks *callbacks)
{
    if (!path || !mode || mode > 7U || !callbacks || !callbacks->transfer)
    {
        return STYXRPC_INVALID;
    }
    path += path[0] == '/';
    if (!*path || strlen(path) > STYXRPC_NODE_PATH_MAX)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcNodes *candidate = *nodes ? *nodes : calloc(1, sizeof(*candidate));
    if (!candidate || !stage(candidate, next, path, mode, callbacks))
    {
        if (!*nodes)
        {
            free(candidate);
        }
        return STYXRPC_INVALID;
    }
    *nodes = candidate;
    return STYXRPC_OK;
}


/* EN/RU: See nodes_internal.h. / См. nodes_internal.h. */
void styxrpc_nodes_dispose(StyxRpcNodes *nodes)
{
    if (nodes)
    {
        for (size_t i = 0; i < nodes->count; ++i)
        {
            StyxRpcNodeCallbacks *callback = &nodes->entries[i].callback;
            if (callback->release)
            {
                callback->release(callback->context);
            }
        }
        free(nodes);
    }
}


/* EN: Find an exact live node by ID, including while old bare walk fids exist.
 * RU: Найти точный живой узел по ID, включая время жизни старых fid обхода. */
static CallbackNode *find(StyxRpcNodes *nodes, uint64_t id)
{
    if (nodes)
    {
        for (size_t i = 0; i < nodes->count; ++i)
        {
            if (nodes->entries[i].node.entry.id == id)
            {
                return nodes->entries + i;
            }
        }
    }
    return NULL;
}


/* EN: Copy node metadata, leaving its name owned by context.
 * RU: Скопировать метаданные узла, сохраняя имя у context. */
static int describe(void *context, uint64_t id, StyxRpcNode *node)
{
    CallbackNode *entry = find(context, id);
    if (entry)
    {
        *node = entry->node;
    }
    return entry != NULL;
}


/* EN: Return index-th child of directory in append order, or NULL.
 * RU: Вернуть потомка directory по индексу в порядке добавления либо NULL. */
static const StyxRpcEntry *enumerate(void *context, uint64_t directory, size_t index)
{
    StyxRpcNodes *nodes = context;
    for (size_t i = 0; nodes && i < nodes->count; ++i)
    {
        CallbackNode *entry = nodes->entries + i;
        if (entry->node.parent == directory && !index--)
        {
            return &entry->node.entry;
        }
    }
    return NULL;
}


/* EN: Pin the selected live file through the slot's owned open list.
 * RU: Удержать выбранный живой файл через список открытий слота. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    CallbackNode *entry = find(context, id);
    if (!entry || !entry->callback.transfer || (access & ~(STYXRPC_READ | STYXRPC_WRITE)))
    {
        return 0;
    }
    *cookie = entry;
    return 1;
}


/* EN: Dispatch exactly one read request into destination bytes. The callback tracks application state.
 * RU: Передать ровно один запрос чтения в буфер назначения. Состояние зависит от приложения. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t size, size_t *used)
{
    CallbackNode *entry = cookie;
    (void)offset;
    return entry->callback.transfer(entry->callback.context, STYXRPC_READ, bytes, size, used);
}


/* EN: Submit one write with an immutable input view; transfer reports an acknowledged prefix.
 * RU: Передать одну запись с неизменяемым входом; transfer подтверждает префикс. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    CallbackNode *entry = cookie;
    (void)offset;
    return entry->callback.transfer(entry->callback.context, STYXRPC_WRITE, (void *)bytes, size, used);
}


/* EN: The borrowed cookie is owned by the registry and needs no individual cleanup.
 * RU: Заимствованный cookie принадлежит реестру и отдельно не освобождается. */
static void close_file(void *cookie)
{
    (void)cookie;
}

const StyxRpcTree styxrpc_nodes_tree = {{enumerate, open_file, read_file, write_file, close_file}, describe}; /* Callback tree for this registry. */
