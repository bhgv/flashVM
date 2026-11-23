/* Copyright (c) 2026 bhgv
 * EN: Lend a rooted local channel graph while translating only generation-specific keys.
 * RU: Передача удерживаемого графа локальных каналов с заменой ключей поколения.
 */
#include "channel_lend.h"
#include "managed_values.h"
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Root one queue and reserve an exact temporary reverse key without accessing script members.
 * RU: Удержать очередь и выделить точный временный ключ без доступа к свойствам AS. */
static FVM_StyxLentQueue *stage(FVM_Context *context, FVM_GCRootScope *roots,
    FVM_StyxLentChannels *channels, const FVM_Value *value)
{
    StyxRpcChannelQueue *queue = fvm_styxrpc_local_channel_queue(context, value);
    FVM_StyxLentQueue *lane;
    size_t index;
    if (!queue)
    {
        return NULL;
    }
    for (index = 0; index < channels->count && channels->lanes[index].local != queue; ++index)
    {
    }
    lane = channels->lanes + index;
    if (index != channels->count)
    {
        return lane;
    }
    if (index == STYXRPC_CHANNEL_SLOTS || fvm_gc_roots_add(roots, value) != FVM_OK)
    {
        return NULL;
    }
    lane->owner = *value;
    lane->local = queue;
    lane->nested = strchr(queue->grant.shape, 'c') != NULL;
    if (lane->nested)
    {
        lane->shadow = *queue;
        lane->shadow.sizes = lane->sizes;
        lane->shadow.bytes = NULL;
    }
    if (styxrpc_argument_scope_nested(channels->scope, lane->nested ? &lane->shadow : queue,
        &lane->grant) != STYXRPC_OK)
    {
        return NULL;
    }
    ++channels->count;
    return lane;
}


/* [GC:] EN/RU: See channel_lend.h. / См. channel_lend.h. */
int fvm_styxrpc_lend_argument(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    FVM_StyxLentChannels *channels = state;
    const char *end;
    size_t length;
    FVM_StyxLentQueue *lane;
    if (value->type == FVM_VALUE_NULL)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    end = fvm_styxrpc_type_end(type);
    lane = stage(context, roots, channels, value);
    if (!end || !lane)
    {
        return 0;
    }
    length = (size_t)(end - type) - 3;
    return strlen(lane->grant.shape) == length + 2 && !memcmp(lane->grant.shape + 1, type + 2, length) &&
        styxrpc_argument_scope_value(channels->scope, lane->nested ? &lane->shadow : lane->local,
            bytes, capacity, used) == STYXRPC_OK;
}


/* [GC:] EN: Remap one local slot and reserve keys for its actual queued descendants.
 * RU: Заменить ключи слота и выделить ключи его фактических потомков в очереди. */
static int prepare_slot(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxLentChannels *channels,
    FVM_StyxLentQueue *lane, size_t slot)
{
    FVM_StyxLocalRefs refs = {0};
    StyxRpcChannelKeyMap keys[STYXRPC_CHANNEL_SLOTS];
    size_t count, offset = slot * lane->local->stride;
    fvm_styxrpc_local_slot(context, lane->owner.object, slot, &refs, 0);
    for (count = 0; count < STYXRPC_CHANNEL_SLOTS && refs.objects[count]; ++count)
    {
        FVM_Value value;
        FVM_StyxLentQueue *child;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = refs.objects[count];
        child = stage(context, roots, channels, &value);
        if (!child)
        {
            return 0;
        }
        keys[count].source = refs.keys[count];
        keys[count].target = child->grant.key;
    }
    return styxrpc_channel_element_remap(lane->grant.shape, lane->local->bytes + offset,
        lane->local->sizes[slot], keys, count, lane->shadow.bytes + offset,
        lane->shadow.stride, lane->sizes + slot) == STYXRPC_OK;
}


/* [GC:] EN/RU: See channel_lend.h. / См. channel_lend.h. */
int fvm_styxrpc_lend_prepare(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxLentChannels *channels)
{
    size_t i, j;
    for (i = 0; i < channels->count; ++i)
    {
        FVM_StyxLentQueue *lane = channels->lanes + i;
        StyxRpcChannelQueue *queue = lane->local;
        if (fvm_styxrpc_local_channel_queue(context, &lane->owner) != queue)
        {
            return 0;
        }
        if (!lane->nested || !queue->capacity)
        {
            continue;
        }
        lane->shadow.head = queue->head;
        lane->shadow.count = queue->count;
        lane->shadow.bytes = malloc(queue->capacity * queue->stride);
        if (!lane->shadow.bytes)
        {
            return 0;
        }
        for (j = 0; j < queue->count; ++j)
        {
            if (!prepare_slot(context, roots, channels, lane, (queue->head + j) % queue->capacity))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN: Recover exact local channels from a validated retained wire slot, without allocation.
 * RU: Восстановить точные локальные каналы проверенного сохранённого слота без выделения памяти. */
static int restore_slot(const FVM_StyxLentChannels *channels, const FVM_StyxLentQueue *lane,
    size_t slot, FVM_StyxLocalRefs *refs)
{
    StyxRpcChannelRoutes routes;
    const StyxRpcChannelQueue *shadow = &lane->shadow;
    size_t i;
    if (styxrpc_channel_element_routes(lane->grant.shape, shadow->bytes + slot * shadow->stride,
        shadow->sizes[slot], &routes) != STYXRPC_OK)
    {
        return 0;
    }
    for (i = 0; i < routes.count; ++i)
    {
        const StyxRpcChannelRoute *route = routes.entries + i;
        size_t child;
        for (child = 0; child < channels->count && channels->lanes[child].grant.key != route->channel.key; ++child)
        {
        }
        if (route->kind != STYXRPC_ROUTE_CURRENT || route->unbuffered || child == channels->count ||
            strcmp(channels->lanes[child].grant.shape, route->channel.shape))
        {
            return 0;
        }
        refs->keys[i] = route->channel.key;
        refs->objects[i] = channels->lanes[child].owner.object;
    }
    return 1;
}


/* EN/RU: See channel_lend.h. / См. channel_lend.h. */
int fvm_styxrpc_lend_restore(FVM_Context *context, FVM_StyxLentChannels *channels)
{
    FVM_StyxLocalRefs restored[STYXRPC_CHANNEL_SLOTS][FVM_STYXRPC_CHANNEL_QUEUE_SLOTS] = {0};
    size_t index, slot;
    for (index = 0; index < channels->count; ++index)
    {
        const FVM_StyxLentQueue *lane = channels->lanes + index;
        const StyxRpcChannelQueue *shadow = &lane->shadow;
        if (lane->nested)
        {
            for (slot = 0; slot < shadow->count; ++slot)
            {
                size_t position = (shadow->head + slot) % shadow->capacity;
                if (!restore_slot(channels, lane, position, &restored[index][position]))
                {
                    return 0;
                }
            }
        }
    }
    for (index = 0; index < channels->count; ++index)
    {
        FVM_StyxLentQueue *lane = channels->lanes + index;
        StyxRpcChannelQueue *local = lane->local;
        const StyxRpcChannelQueue *shadow = &lane->shadow;
        if (!lane->nested || !shadow->capacity)
        {
            continue;
        }
        local->head = shadow->head;
        local->count = shadow->count;
        for (slot = 0; slot < shadow->capacity; ++slot)
        {
            size_t age = (slot + shadow->capacity - shadow->head) % shadow->capacity;
            if (age < shadow->count)
            {
                memcpy(local->bytes + slot * local->stride, shadow->bytes + slot * shadow->stride,
                    shadow->sizes[slot]);
                local->sizes[slot] = shadow->sizes[slot];
            }
            fvm_styxrpc_local_slot(context, lane->owner.object, slot, &restored[index][slot], 1);
        }
    }
    return 1;
}


/* EN/RU: See channel_lend.h. / См. channel_lend.h. */
void fvm_styxrpc_lend_clear(FVM_StyxLentChannels *channels)
{
    size_t i;
    for (i = 0; i < channels->count; ++i)
    {
        free(channels->lanes[i].shadow.bytes);
    }
}
