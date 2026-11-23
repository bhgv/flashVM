/* Copyright (c) 2026 bhgv
 * EN: Manage script-local native arenas and resolve retired PID graph keys on the owner.
 * RU: Владение локальными аренами и восстановление ключей отозванного PID у владельца.
 */
#include "channel_local.h"
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Drop the userdata or publication reference to this native arena.
 * RU: Снять ссылку userdata либо публикации на этот буфер. */
void fvm_styxrpc_local_channel_release(void *state)
{
    LocalChannel *local = state;
    if (!--local->references)
    {
        free(local);
    }
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
StyxRpcChannelQueue *fvm_styxrpc_local_channel_queue(FVM_Context *context, const FVM_Value *value)
{
    LocalChannel *local = value->type == FVM_VALUE_OBJECT ?
        fvm_styxrpc_local_channel_lookup(context, value->object) : NULL;
    return local && !local->closed && !local->reading && !local->lent &&
        fvm_styxrpc_local_sync(context, value->object, local) ? &local->queue : NULL;
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
StyxRpcChannelQueue *fvm_styxrpc_local_channel_borrow(FVM_Context *context, const FVM_Value *value, void **owner)
{
    LocalChannel *local = value && value->type == FVM_VALUE_OBJECT ?
        fvm_styxrpc_local_channel_lookup(context, value->object) : NULL;
    if (!local || local->closed || local->reading || local->writing || local->lent ||
        !fvm_styxrpc_local_sync(context, value->object, local))
    {
        return NULL;
    }
    ++local->references;
    local->lent = 1;
    *owner = local;
    return &local->queue;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_local_channel_return(void *owner, int issued)
{
    LocalChannel *local = owner;
    local->restore = issued;
    local->lent = 0;
    fvm_styxrpc_local_channel_release(local);
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_local_slot(FVM_Context *context, FVM_Object *owner, size_t slot, FVM_StyxLocalRefs *refs, int store)
{
    LocalChannel *local = fvm_styxrpc_local_channel_lookup(context, owner);
    size_t index;
    unsigned bit = 1U << slot;
    if (store)
    {
        local->issued = (local->issued & ~bit) | (refs->issued ? bit : 0);
    }
    else
    {
        refs->issued = (local->issued & bit) != 0;
    }
    for (index = 0; index < STYXRPC_CHANNEL_SLOTS; ++index)
    {
        if (store)
        {
            local->keys[slot][index] = refs->keys[index];
        }
        else
        {
            refs->keys[index] = local->keys[slot][index];
        }
        refs->objects[index] = fvm_styxrpc_local_edge(owner, slot, index, refs->objects[index], store);
    }
}


/* [GC:] EN: Resolve a checked wire slot through graph without allocating or running AS.
 * RU: Разрешить проверенный сетевой слот через graph без аллокатора и вызова AS. */
static int decode_slot(FVM_Context *context, FVM_Object *graph, const StyxRpcChannelQueue *queue,
    size_t slot, FVM_StyxLocalRefs *refs)
{
    StyxRpcChannelRoutes routes;
    size_t index;
    if (styxrpc_channel_element_routes(queue->grant.shape, queue->bytes + slot * queue->stride,
        queue->sizes[slot], &routes) != STYXRPC_OK)
    {
        return 0;
    }
    refs->issued = 1;
    for (index = 0; index < routes.count; ++index)
    {
        const StyxRpcChannelRoute *route = routes.entries + index;
        FVM_Object *object = fvm_styxrpc_local_graph_node(graph, route->channel.key);
        const LocalChannel *child = fvm_styxrpc_local_channel_lookup(context, object);
        if (route->kind != STYXRPC_ROUTE_CURRENT || !child ||
            route->unbuffered != !child->queue.capacity || strcmp(route->channel.shape, child->queue.grant.shape))
        {
            return 0;
        }
        refs->keys[index] = route->channel.key;
        refs->objects[index] = object;
    }
    return 1;
}


/* [GC:] EN/RU: See channel_local.h. / См. channel_local.h. */
int fvm_styxrpc_local_sync(FVM_Context *context, FVM_Object *owner, LocalChannel *local)
{
    StyxRpcChannelQueue *queue = &local->queue;
    FVM_StyxLocalRefs refs[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS] = {0};
    FVM_Object *graph;
    size_t index;
    if (!local->restore)
    {
        return 1;
    }
    graph = fvm_styxrpc_local_edge(owner, queue->capacity, 0, NULL, 0);
    for (index = 0; index < queue->count; ++index)
    {
        size_t slot = (queue->head + index) % queue->capacity;
        if (!decode_slot(context, graph, queue, slot, refs + slot))
        {
            return 0;
        }
    }
    for (index = 0; index < queue->capacity; ++index)
    {
        fvm_styxrpc_local_slot(context, owner, index, refs + index, 1);
    }
    fvm_styxrpc_local_edge(owner, queue->capacity, 0, NULL, 1);
    local->restore = 0;
    return 1;
}
