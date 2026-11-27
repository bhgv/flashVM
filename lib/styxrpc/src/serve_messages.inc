/* Copyright (c) 2026 bhgv
 * EN: Publish, reserve and retire live exchange roots on a hosted Styx tree.
 * RU: Публикация, резервирование и отзыв живых корней обмена в дереве Styx.
 */
#include "styxrpc/vfs_client.h"


/* EN: Exchange names are single root components. Allow protocol exchange names.
 * RU: Имя обмена состоит из одного компонента корня. Имена протокола допустимы. */
static int exchange_name(const char *name)
{
    size_t size = strlen(name);
    return size && size <= STYXRPC_TREE_NAME && !strchr(name, '/') &&
        strcmp(name, ".") && strcmp(name, "..") &&
        (!styxrpc_vfs_service_name(name, size) || !strcmp(name, STYXRPC_UNIBUS_FILE) ||
         !strcmp(name, STYXRPC_MESSAGES_FILE) || !strcmp(name, STYXRPC_EXCHANGE_DEFAULT));
}


/* EN: Drop a registration and its reservation, retaining the slot generation.
 * RU: Снять регистрацию и резерв, сохраняя поколение слота. */
static void retire_root(StyxRpcServer *server, MessageRoot *root)
{
    MessageService *service = root->service;
    root->service = NULL;
    styxrpc_message_service_destroy(service);
    if (root->claim)
    {
        char path[STYXRPC_TREE_NAME + 2] = "/";
        strcpy(path + 1, root->name);
        styxrpc_slot_drop(server->context, path, root->claim);
        root->claim = 0;
    }
}


/* EN/RU: See messages.h. / См. messages.h. */
StyxRpcStatus styxrpc_messages_publish(StyxRpcServer *server, const char *name,
    const StyxRpcMessageSpec *spec)
{
    name = name ? name : STYXRPC_UNIBUS_FILE;
    if (!server || !exchange_name(name))
    {
        return STYXRPC_INVALID;
    }
    size_t slot = STYXRPC_MESSAGE_ROOTS_MAX;
    for (size_t i = 0; i < STYXRPC_MESSAGE_ROOTS_MAX; ++i)
    {
        const MessageRoot *root = &server->messages[i];
        if (root->service && !strcmp(name, root->name))
        {
            return STYXRPC_INVALID;
        }
        if (!root->service && !root->claim && root->generation < UINT32_MAX && slot == STYXRPC_MESSAGE_ROOTS_MAX)
        {
            slot = i;
        }
    }
    if (slot == STYXRPC_MESSAGE_ROOTS_MAX)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcNode node = {.entry = {STYXRPC_MESSAGE_NODE - slot, name, 0666, 0}};
    MessageService *service = styxrpc_message_service(server->tree, server->context, &node, spec);
    if (!service)
    {
        return STYXRPC_INVALID;
    }
    MessageRoot *root = &server->messages[slot];
    if (server->tree == &styxrpc_module_slot_tree)
    {
        char path[STYXRPC_TREE_NAME + 2] = "/";
        strcpy(path + 1, name);
        root->claim = styxrpc_slot_claim(server->context, path);
        if (!root->claim)
        {
            styxrpc_message_service_destroy(service);
            return STYXRPC_INVALID;
        }
    }
    strcpy(root->name, name);
    root->service = service;
    ++root->generation;
    return STYXRPC_OK;
}


/* EN/RU: See messages.h. / См. messages.h. */
StyxRpcStatus styxrpc_messages_withdraw(StyxRpcServer *server, const char *name)
{
    name = name ? name : STYXRPC_UNIBUS_FILE;
    if (!server)
    {
        return STYXRPC_INVALID;
    }
    for (size_t i = 0; i < STYXRPC_MESSAGE_ROOTS_MAX; ++i)
    {
        MessageRoot *root = &server->messages[i];
        if (root->service && !strcmp(name, root->name))
        {
            retire_root(server, root);
            return STYXRPC_OK;
        }
    }
    return STYXRPC_INVALID;
}


/* EN/RU: See messages.h. / См. messages.h. */
StyxRpcStatus styxrpc_messages_enable(StyxRpcServer *server, const StyxRpcMessageSpec *spec)
{
    return server && !server->started ? styxrpc_messages_publish(server, STYXRPC_MESSAGES_FILE, spec) :
        STYXRPC_INVALID;
}
