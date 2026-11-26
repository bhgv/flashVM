/* Copyright (c) 2026 bhgv
 * EN: Reserve named channel paths against the live callback namespace.
 * RU: Резервирование имён каналов с учётом живого пространства колбэков.
 */
#include "module_slot_internal.h"
#include <stdlib.h>
#include <string.h>

struct SlotClaim
{
    SlotClaim *next;                /* Next reservation, protected by slot->names. */
    uint64_t id;                    /* Exact reservation identity. */
    uint64_t parent;                /* Live directory identity at enrollment. */
    char path[STYXRPC_VFS_PATH_MAX]; /* Absolute, canonical path ending in NUL. */
};


/* EN: Resolve the parent of a canonical path while the names lock is held.
 * RU: Найти родительский узел канонического пути под блокировкой имён. */
static int parent_id(const StyxRpcModuleSlot *slot, const char *path, uint64_t *id)
{
    const char *last = strrchr(path, '/');
    if (last == path)
    {
        *id = 0;
        return 1;
    }
    char parent[STYXRPC_VFS_PATH_MAX];
    size_t length = (size_t)(last - path - 1);
    memcpy(parent, path + 1, length);
    parent[length] = 0;
    StyxRpcNode node;
    if (!styxrpc_nodes_lookup_path(slot->nodes, parent, &node) || !node.entry.directory || !(node.entry.mode & 1U))
    {
        return 0;
    }
    *id = node.entry.id;
    return 1;
}


/* EN: Test a component-aligned prefix of one relative path against another.
 * RU: Проверить префикс одного относительного пути по границе компонента другого. */
static int prefix(const char *path, const char *candidate)
{
    size_t length = strlen(candidate);
    return !strncmp(path, candidate, length) && (!path[length] || path[length] == '/');
}


/* EN/RU: See module_slot_internal.h. / См. module_slot_internal.h. */
int styxrpc_slot_claim_conflict(const StyxRpcModuleSlot *slot, const char *path)
{
    if (!path)
    {
        return 0;
    }
    path += *path == '/';
    for (const SlotClaim *claim = slot->claims; claim; claim = claim->next)
    {
        if (prefix(path, claim->path + 1))
        {
            return 1;
        }
    }
    return 0;
}


/* EN: Admit one missing ordinary name in a live directory. This reserves a
 * rendezvous name outside the ordinary file permissions and directory listing.
 * RU: Зарезервировать свободное имя в живом каталоге. Резерв канала действует
 * отдельно от прав записи файлов и перечисления каталога. */
static uint64_t claim_path(void *context, const char *path)
{
    StyxRpcModuleSlot *slot = context;
    uint64_t id = 0, parent;
    StyxRpcNode occupied;
    pthread_mutex_lock(&slot->names);
    if (!prefix(path + 1, "mounted") && slot->last_claim < UINT64_MAX &&
        parent_id(slot, path, &parent) && !styxrpc_nodes_lookup_path(slot->nodes, path + 1, &occupied) &&
        !styxrpc_slot_claim_conflict(slot, path))
    {
        SlotClaim *claim = malloc(sizeof(*claim));
        if (claim)
        {
            claim->id = id = ++slot->last_claim;
            claim->parent = parent;
            strcpy(claim->path, path);
            claim->next = slot->claims;
            slot->claims = claim;
        }
    }
    pthread_mutex_unlock(&slot->names);
    return id;
}


/* EN: Join only while the exact reserved parent directory remains live.
 * RU: Допустить нового участника, пока жив точный родительский каталог резерва. */
static int visible_path(void *context, const char *path, uint64_t id)
{
    StyxRpcModuleSlot *slot = context;
    pthread_mutex_lock(&slot->names);
    const SlotClaim *claim = slot->claims;
    while (claim && claim->id != id)
    {
        claim = claim->next;
    }
    uint64_t parent;
    int live = claim && parent_id(slot, path, &parent) && parent == claim->parent;
    pthread_mutex_unlock(&slot->names);
    return live;
}


/* EN: Drop exactly the last participant's reservation for path and id.
 * RU: Освободить точный резерв последнего участника по path и id. */
static void drop_path(void *context, const char *path, uint64_t id)
{
    StyxRpcModuleSlot *slot = context;
    pthread_mutex_lock(&slot->names);
    SlotClaim **link = &slot->claims;
    while (*link && ((*link)->id != id || strcmp((*link)->path, path)))
    {
        link = &(*link)->next;
    }
    if (*link)
    {
        SlotClaim *claim = *link;
        *link = claim->next;
        free(claim);
    }
    pthread_mutex_unlock(&slot->names);
}


/* EN/RU: See module_slot_internal.h. / См. module_slot_internal.h. */
void styxrpc_slot_claims_clear(StyxRpcModuleSlot *slot)
{
    while (slot->claims)
    {
        SlotClaim *claim = slot->claims;
        slot->claims = claim->next;
        free(claim);
    }
}


/* EN/RU: See module_slot.h. / См. module_slot.h. */
StyxRpcNamedRegistry *styxrpc_module_slot_named_registry(StyxRpcModuleSlot *slot)
{
    return slot ? styxrpc_named_registry(claim_path, visible_path, drop_path, slot) : NULL;
}
