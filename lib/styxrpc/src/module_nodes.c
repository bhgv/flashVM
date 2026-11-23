/* Copyright (c) 2026 bhgv
 * EN: Build immutable hosted module nodes and describe them by logical ID.
 * RU: Создание неизменяемых узлов хостового модуля с логическими ID.
 */
#include "module_nodes.h"
#include <string.h>


/* EN/RU: See module_nodes.h. / См. module_nodes.h. */
int styxrpc_module_name(const char *name)
{
    return name && *name && strlen(name) <= STYXRPC_TREE_NAME &&
        !strchr(name, '/') && strcmp(name, ".") && strcmp(name, "..");
}


/* EN/RU: See module_nodes.h. / См. module_nodes.h. */
void styxrpc_module_nodes(StyxRpcEntry *entries, const char *name)
{
    entries[ROOT] = (StyxRpcEntry){ROOT, "/", 0555, 1};
    entries[MOUNTED] = (StyxRpcEntry){MOUNTED, "mounted", 0555, 1};
    entries[MODULE] = (StyxRpcEntry){MODULE, name, 0555, 1};
    entries[INTERFACE] = (StyxRpcEntry){INTERFACE, ".interface", 0444, 0};
}


/* EN/RU: See module_nodes.h. / См. module_nodes.h. */
void styxrpc_module_export_node(StyxRpcEntry *entries, size_t index, const char *name)
{
    size_t id = FIRST_EXPORT + index;
    entries[id] = (StyxRpcEntry){id, name, 0777, 0};
}


/* EN/RU: See module_nodes.h. / См. module_nodes.h. */
const StyxRpcEntry *styxrpc_module_child(const StyxRpcEntry *entries, size_t count,
    uint64_t directory, size_t index)
{
    if ((directory == ROOT || directory == MOUNTED) && !index)
    {
        return entries + directory + 1;
    }
    return directory == MODULE && index <= count ? entries + INTERFACE + index : NULL;
}


/* EN/RU: See module_nodes.h. / См. module_nodes.h. */
int styxrpc_module_stat(const StyxRpcEntry *entries, size_t count, size_t profile_size,
    uint64_t id, StyxRpcNode *node)
{
    if (id >= FIRST_EXPORT + count)
    {
        return 0;
    }
    *node = (StyxRpcNode){.entry = entries[id],
        .parent = id <= MOUNTED ? ROOT : id == MODULE ? MOUNTED : MODULE,
        .length = id == INTERFACE ? profile_size : 0};
    return 1;
}
