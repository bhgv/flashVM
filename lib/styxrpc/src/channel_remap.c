/* Copyright (c) 2026 bhgv
 * EN: Reframe a checked typed element after mapping its current-bus keys.
 * RU: Пересборка проверенного типизированного элемента после замены ключей текущей шины.
 */
#include "channel_remap.h"
#include "call_record.h"
#include "styxrpc/channel_call.h"
#include <stdlib.h>
#include <string.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Maximum typed element extent. */


/* EN: Copy or canonically reframe one record from a checked typed element.
 * RU: Скопировать или заново оформить одну запись проверенного типизированного элемента. */
static int rewrite(const ChannelKeyTransform *mapping, const unsigned char *bytes, const FVM_SRecord *record,
    size_t extent, unsigned depth, unsigned char *output, size_t capacity, size_t *used)
{
    if ((record->tag == STYX_MANAGED_CHANNEL_TAG || record->tag == STYX_MANAGED_UNBUFFERED_CHANNEL_TAG) &&
        record->size == 4)
    {
        const unsigned char *p = record->payload;
        uint32_t key = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
        int unbuffered = record->tag == STYX_MANAGED_UNBUFFERED_CHANNEL_TAG;
        if (!mapping->apply(mapping->context, &key, &unbuffered))
        {
            return 0;
        }
        if (mapping->result)
        {
            return styxrpc_channel_export_value(mapping->token, key, unbuffered,
                output, capacity, used) == STYXRPC_OK;
        }
        unsigned char leaf[] = {key >> 24, key >> 16, key >> 8, key};
        return fvm_srecord_write(unbuffered ? STYX_MANAGED_UNBUFFERED_CHANNEL_TAG : STYX_MANAGED_CHANNEL_TAG,
            leaf, sizeof(leaf), output, capacity, used) == FVM_S_OK;
    }
    if (mapping->result && (record->tag == STYX_MANAGED_EXPORTED_CHANNEL_TAG ||
        record->tag == STYX_MANAGED_CHANNEL_MODULE_GRANT_TAG || record->tag == STYX_MANAGED_CHANNEL_GRANT_TAG))
    {
        return 0;
    }
    int children = record->tag == STYXRPC_CHANNEL_ELEMENT_TAG ||
        record->tag == STYXRPC_VALUE_ARRAY || record->tag == STYXRPC_VALUE_TUPLE;
    if (!children || !record->size)
    {
        if (extent > capacity)
        {
            return 0;
        }
        memcpy(output, bytes, extent);
        *used = extent;
        return 1;
    }
    if (depth == STYXRPC_MANAGED_SHAPE || !capacity)
    {
        return 0;
    }
    unsigned char *body = malloc(capacity);
    if (!body)
    {
        return 0;
    }
    size_t offset = 0, written = 0;
    int ok = 1;
    while (ok && offset < record->size)
    {
        FVM_SRecord child;
        size_t length, placed;
        const unsigned char *source = record->payload + offset;
        ok = fvm_srecord_read(source, record->size - offset, FRAME, &child, &length) == FVM_S_OK &&
            rewrite(mapping, source, &child, length, depth + 1, body + written, capacity - written, &placed);
        if (ok)
        {
            offset += length;
            written += placed;
        }
    }
    ok = ok && fvm_srecord_write(record->tag, body, written, output, capacity, used) == FVM_S_OK;
    free(body);
    return ok;
}


/* EN/RU: See channel_remap.h. / См. channel_remap.h. */
int styxrpc_channel_reframe(const ChannelKeyTransform *mapping, const void *bytes, size_t size,
    void *output, size_t capacity, size_t *used)
{
    FVM_SRecord outer;
    size_t extent;
    return capacity <= FRAME && fvm_srecord_read(bytes, size, FRAME, &outer, &extent) == FVM_S_OK &&
        extent == size && (mapping->result || outer.tag == STYXRPC_CHANNEL_ELEMENT_TAG) &&
        rewrite(mapping, bytes, &outer, extent, 0, output, capacity, used);
}


typedef struct KeyTable
{
    const StyxRpcChannelKeyMap *keys; /* Borrowed mapping through reframing. */
    size_t count;                      /* Number of valid entries. */
} KeyTable;


/* EN: Replace the key with its generation key and emit the ordinary scoped marker.
 * RU: Заменить ключ ключом поколения и записать обычный контекстный маркер. */
static int map_key(const void *context, uint32_t *key, int *unbuffered)
{
    const KeyTable *table = context;
    for (size_t i = 0; i < table->count; ++i)
    {
        if (table->keys[i].source == *key)
        {
            *key = table->keys[i].target;
            *unbuffered = 0;
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See styxrpc/channel_route.h. / См. styxrpc/channel_route.h. */
StyxRpcStatus styxrpc_channel_element_remap(const char *shape, const void *bytes, size_t size,
    const StyxRpcChannelKeyMap *keys, size_t count, void *output, size_t capacity, size_t *used)
{
    if (!output || !used || count > STYXRPC_CHANNEL_SLOTS || (count && !keys))
    {
        return STYXRPC_INVALID;
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (!keys[i].source || keys[i].source > INT32_MAX || !keys[i].target || keys[i].target > INT32_MAX)
        {
            return STYXRPC_INVALID;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (keys[j].source == keys[i].source || keys[j].target == keys[i].target)
            {
                return STYXRPC_INVALID;
            }
        }
    }
    StyxRpcChannelRoutes routes;
    if (styxrpc_channel_element_routes(shape, bytes, size, &routes) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    for (size_t i = 0; i < routes.count; ++i)
    {
        if (routes.entries[i].kind != STYXRPC_ROUTE_CURRENT)
        {
            return STYXRPC_INVALID;
        }
    }
    const KeyTable table = {keys, count};
    const ChannelKeyTransform mapping = {.apply = map_key, .context = &table};
    return styxrpc_channel_reframe(&mapping, bytes, size, output, capacity, used) ? STYXRPC_OK : STYXRPC_INVALID;
}
