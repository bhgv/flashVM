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
    StyxRpcNodes *owner;           /* Registry pinned by this entry and any opens. */
    NodeRegistration callback; /* Adopted file state, empty for directories. */
    char name[STYXRPC_TREE_NAME + 1]; /* Component owned by this record. */
} CallbackNode;

struct StyxRpcNodes
{
    CallbackNode entries[STYXRPC_NODE_LIMIT]; /* Stable file and directory storage. */
    CallbackNode *ordered[STYXRPC_NODE_LIMIT]; /* Live entries in publication order. */
    size_t count;                             /* Size of the live order. */
    unsigned dispatches;                      /* Nesting of owner callbacks. */
};

/* EN: Resolve one ordinary absolute-from-attach component, rejecting service aliases.
 * RU: Проверить компонент от корня attach и исключить служебные имена. */
static int component(const char *name, size_t length, int root)
{
    if (!length || length > STYXRPC_TREE_NAME || (root && length == 7 && !memcmp(name, "mounted", 7)))
    {
        return 0;
    }
    return !(length == 1 && name[0] == '.') &&
        !(length == 2 && !memcmp(name, "..", 2)) && !styxrpc_vfs_service_name(name, length);
}


/* EN: Locate a child in the published prefix. Temporary additions follow a new parent.
 * RU: Найти потомка в опубликованном префиксе. Временные узлы идут под новым родителем. */
static CallbackNode *child(StyxRpcNodes *nodes, uint64_t parent, const char *name, size_t length)
{
    for (size_t i = 0; nodes && i < nodes->count; ++i)
    {
        CallbackNode *entry = nodes->ordered[i];
        if (entry->node.parent == parent && strlen(entry->name) == length &&
            !memcmp(entry->name, name, length))
        {
            return entry;
        }
    }
    return NULL;
}


/* EN: Resolve a validated relative spelling in the current callback tree.
 * RU: Найти проверенное относительное имя в текущем дереве колбэков. */
static CallbackNode *resolve(StyxRpcNodes *nodes, const char *path)
{
    uint64_t parent = 0;
    for (;;)
    {
        size_t count = strcspn(path, "/");
        CallbackNode *entry = child(nodes, parent, path, count);
        if (!entry || !path[count])
        {
            return entry;
        }
        if (!entry->node.entry.directory)
        {
            return NULL;
        }
        parent = entry->node.entry.id;
        path += count + 1;
    }
}


/* EN: Run one callback and confirm a bounded prefix. Only positioned file I/O clips to EOF.
 * RU: Вызвать один колбэк и подтвердить префикс в границах буфера. Учитывать EOF у файлов со смещением. */
static int transfer(CallbackNode *entry, unsigned access, uint64_t offset,
    void *bytes, size_t size, size_t *used)
{
    NodeRegistration *callback = &entry->callback;
    *used = 0;
    if (callback->positioned && access != STYXRPC_NODE_EXECUTE)
    {
        if (!size)
        {
            return 1;
        }
        uint64_t remaining = offset < callback->length ? callback->length - offset : 0;
        if (remaining < size)
        {
            if (access == STYXRPC_WRITE)
            {
                return 0;
            }
            if (!remaining)
            {
                return 1;
            }
            size = (size_t)remaining;
        }
    }
    ++entry->owner->dispatches;
    int accepted = callback->stream ? callback->stream(callback->context, access, bytes, size, used) :
        callback->positioned(callback->context, access, offset, bytes, size, used);
    --entry->owner->dispatches;
    return accepted == 1 && *used <= size;
}


/* EN/RU: See nodes_internal.h. / См. nodes_internal.h. */
int styxrpc_nodes_exec(void *registry, const char *path,
    void *bytes, size_t size, size_t *used)
{
    CallbackNode *entry = resolve(registry, path);
    return entry && !entry->node.entry.directory && (entry->node.entry.mode & 1U) &&
        transfer(entry, STYXRPC_NODE_EXECUTE, 0, bytes, size, used);
}


/* EN: Find one reusable empty record without moving any live open's cookie.
 * RU: Найти свободную запись для повторного использования без смещения открытых записей. */
static CallbackNode *vacancy(StyxRpcNodes *nodes)
{
    for (size_t i = 0; i < STYXRPC_NODE_LIMIT; ++i)
    {
        if (!nodes->entries[i].node.entry.id)
        {
            return nodes->entries + i;
        }
    }
    return NULL;
}


/* EN: Stage one path, leaving rollback to the caller on failure.
 * RU: Подготовить путь. При отказе вызывающая сторона откатывает добавления. */
static int stage(StyxRpcNodes *nodes, uint64_t *next, const char *path, unsigned mode,
    const NodeRegistration *callbacks)
{
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
            entry = id <= UINT32_MAX ? vacancy(nodes) : NULL;
            if (!entry)
            {
                return 0;
            }
            entry->owner = nodes;
            nodes->ordered[nodes->count++] = entry;
            memcpy(entry->name, path, length);
            entry->node.entry = (StyxRpcEntry){id++, entry->name, directory ? 0555U : mode * 0111U, directory};
            entry->node.parent = parent;
        }
        if (!directory)
        {
            entry->callback = *callbacks;
            entry->node.length = callbacks->length;
            *next = id;
            return 1;
        }
        parent = entry->node.entry.id;
        path += length + 1;
    }
}


/* EN/RU: See nodes_internal.h. / См. nodes_internal.h. */
StyxRpcStatus styxrpc_nodes_insert(StyxRpcNodes **nodes, uint64_t *next, const char *path,
    unsigned mode, const NodeRegistration *callbacks)
{
    if (!path || !mode || mode > 7U || !callbacks || (!callbacks->stream && !callbacks->positioned) ||
        callbacks->length > STYXRPC_NODE_OFFSET_MAX)
    {
        return STYXRPC_INVALID;
    }
    path += path[0] == '/';
    if (!*path || strlen(path) > STYXRPC_NODE_PATH_MAX)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcNodes *candidate = *nodes ? *nodes : calloc(1, sizeof(*candidate));
    if (!candidate)
    {
        return STYXRPC_INVALID;
    }
    size_t before = candidate->count;
    if (!stage(candidate, next, path, mode, callbacks))
    {
        while (candidate->count > before)
        {
            CallbackNode *entry = candidate->ordered[--candidate->count];
            candidate->ordered[candidate->count] = NULL;
            memset(entry, 0, sizeof(*entry));
        }
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
            NodeRegistration *callback = &nodes->ordered[i]->callback;
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
            if (nodes->ordered[i]->node.entry.id == id)
            {
                return nodes->ordered[i];
            }
        }
    }
    return NULL;
}


/* EN/RU: See nodes_internal.h. / См. nodes_internal.h. */
int styxrpc_nodes_busy(const StyxRpcNodes *nodes)
{
    return nodes && nodes->dispatches != 0;
}


/* EN/RU: See nodes_internal.h. / См. nodes_internal.h. */
uint64_t styxrpc_nodes_removable(StyxRpcNodes *nodes, const char *path)
{
    if (!nodes || styxrpc_nodes_busy(nodes) || !path)
    {
        return 0;
    }
    path += path[0] == '/';
    if (!*path || strlen(path) > STYXRPC_NODE_PATH_MAX)
    {
        return 0;
    }
    CallbackNode *entry = resolve(nodes, path);
    return entry && !entry->node.entry.directory ? entry->node.entry.id : 0;
}


/* EN: Erase one live record and its ordered-list membership, without moving its neighbors.
 * RU: Стереть живую запись и её место в списке, сохранив адреса соседей. */
static void erase(StyxRpcNodes *nodes, CallbackNode *entry)
{
    size_t index = 0;
    while (nodes->ordered[index] != entry)
    {
        ++index;
    }
    --nodes->count;
    memmove(nodes->ordered + index, nodes->ordered + index + 1,
        (nodes->count - index) * sizeof(*nodes->ordered));
    nodes->ordered[nodes->count] = NULL;
    memset(entry, 0, sizeof(*entry));
}


/* EN: Test whether a live directory still holds another registered entry.
 * RU: Проверить наличие других зарегистрированных записей в живом каталоге. */
static int has_children(StyxRpcNodes *nodes, uint64_t id)
{
    for (size_t i = 0; i < nodes->count; ++i)
    {
        if (nodes->ordered[i]->node.parent == id)
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See nodes_internal.h. / См. nodes_internal.h. */
void styxrpc_nodes_remove(StyxRpcNodes *nodes, uint64_t id)
{
    CallbackNode *entry = find(nodes, id);
    if (!entry)
    {
        return;
    }
    NodeRegistration callback = entry->callback;
    uint64_t parent = entry->node.parent;
    erase(nodes, entry);
    while (parent && !has_children(nodes, parent))
    {
        entry = find(nodes, parent);
        parent = entry->node.parent;
        erase(nodes, entry);
    }
    if (callback.release)
    {
        callback.release(callback.context);
    }
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
        CallbackNode *entry = nodes->ordered[i];
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
    if (!entry || entry->node.entry.directory || (access & ~(STYXRPC_READ | STYXRPC_WRITE)))
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
    return transfer(cookie, STYXRPC_READ, offset, bytes, size, used);
}


/* EN: Submit one write with an immutable input view; transfer reports an acknowledged prefix.
 * RU: Передать одну запись с неизменяемым входом; transfer подтверждает префикс. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    return transfer(cookie, STYXRPC_WRITE, offset, (void *)bytes, size, used);
}


/* EN: The borrowed cookie is owned by the registry and needs no individual cleanup.
 * RU: Заимствованный cookie принадлежит реестру и отдельно не освобождается. */
static void close_file(void *cookie)
{
    (void)cookie;
}

const StyxRpcTree styxrpc_nodes_tree = {{enumerate, open_file, read_file, write_file, close_file}, describe}; /* Callback tree for this registry. */
