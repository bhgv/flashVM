/* Copyright (c) 2026 bhgv
 * EN: Collect and encode addresses from the existing Dis channel message dialect.
 * RU: Сбор и кодирование адресов существующего формата сообщений каналов Dis.
 */
#include "channel_route_internal.h"
#include "call_record.h"
#include "load_profile.h"
#include <string.h>

#define REVERSE_BIT UINT32_C(0x80000000) /* Reverse PID namespace and wire capacity flag. */

static const unsigned char zero_token[STYXRPC_CHANNEL_TOKEN_SIZE]; /* Empty identity sentinel. */


/* EN: Decode a four-byte network-order value from checked storage.
 * RU: Декодировать четырёхбайтовое значение из проверенного буфера. */
static uint32_t read_word(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 | (uint32_t)bytes[2] << 8 | bytes[3];
}


/* EN: Encode one network-order value in a four-byte destination.
 * RU: Записать одно значение в четырёхбайтовый буфер сетевого порядка. */
static void write_word(unsigned char *bytes, uint32_t word)
{
    bytes[0] = (unsigned char)(word >> 24);
    bytes[1] = (unsigned char)(word >> 16);
    bytes[2] = (unsigned char)(word >> 8);
    bytes[3] = (unsigned char)word;
}


/* EN/RU: See channel_route_internal.h. / См. channel_route_internal.h. */
int styxrpc_route_valid(const StyxRpcChannelRoute *route)
{
    if (!route || !route->channel.key || route->channel.key >= REVERSE_BIT ||
        (route->unbuffered != 0 && route->unbuffered != 1) ||
        !memchr(route->channel.shape, 0, sizeof(route->channel.shape)) ||
        route->channel.shape[0] != '(' || strchr(route->channel.shape, 'h'))
    {
        return 0;
    }
    const char *end = styxshape_end(route->channel.shape + 1);
    if (!end || strcmp(end, ")"))
    {
        return 0;
    }
    int token = memcmp(route->token, zero_token, sizeof(zero_token)) != 0;
    switch (route->kind)
    {
    case STYXRPC_ROUTE_CURRENT:
        return !route->pid && !token;
    case STYXRPC_ROUTE_PID:
        return route->pid >= REVERSE_BIT && !token;
    case STYXRPC_ROUTE_MODULE:
        return !route->pid && token;
    }
    return 0;
}


/* EN: Decode just the leaf address and derive the element shape from its schema.
 * RU: Декодировать адрес листа и вывести форму элемента из схемы. */
static int decode(const char *type, const FVM_SRecord *leaf, StyxRpcChannelRoute *route, int exported)
{
    const char *end = styxshape_end(type);
    if (exported != (leaf->tag == STYX_MANAGED_EXPORTED_CHANNEL_TAG) || !end || *type != 'c' || end - type < 4 || (size_t)(end - type) > sizeof(route->channel.shape))
    {
        return 0;
    }
    size_t prefix;
    switch (leaf->tag)
    {
    case STYX_MANAGED_CHANNEL_TAG:
    case STYX_MANAGED_UNBUFFERED_CHANNEL_TAG:
        prefix = 0;
        break;
    case STYX_MANAGED_CHANNEL_GRANT_TAG:
        prefix = 4;
        route->kind = STYXRPC_ROUTE_PID;
        break;
    case STYX_MANAGED_CHANNEL_MODULE_GRANT_TAG:
    case STYX_MANAGED_EXPORTED_CHANNEL_TAG:
        prefix = sizeof(route->token);
        route->kind = STYXRPC_ROUTE_MODULE;
        break;
    default:
        return 0;
    }
    if (leaf->size != prefix + 4)
    {
        return 0;
    }
    uint32_t key = read_word(leaf->payload + prefix);
    if (!prefix && (key & REVERSE_BIT))
    {
        return 0;
    }
    if (route->kind == STYXRPC_ROUTE_PID)
    {
        route->pid = read_word(leaf->payload);
    }
    else if (route->kind == STYXRPC_ROUTE_MODULE)
    {
        memcpy(route->token, leaf->payload, prefix);
    }
    route->channel.key = key & INT32_MAX;
    route->unbuffered = (key >> 31) || leaf->tag == STYX_MANAGED_UNBUFFERED_CHANNEL_TAG;
    size_t length = (size_t)(end - type) - 3;
    route->channel.shape[0] = '(';
    memcpy(route->channel.shape + 1, type + 2, length);
    route->channel.shape[length + 1] = ')';
    return styxrpc_route_valid(route);
}


typedef struct RouteList
{
    StyxRpcChannelRoutes routes; /* Staging table published only on complete success. */
    int exported;                /* Accept result-only records when set. */
} RouteList;


/* EN: Add a typed leaf, coalescing exact aliases within this one graph.
 * RU: Добавить типизированный лист, объединяя точные алиасы одного графа. */
static int collect(void *context, const char *type, const FVM_SRecord *leaf)
{
    RouteList *list = context;
    StyxRpcChannelRoutes *routes = &list->routes;
    StyxRpcChannelRoute route = {0};
    if (!decode(type, leaf, &route, list->exported))
    {
        return 0;
    }
    for (size_t i = 0; i < routes->count; ++i)
    {
        const StyxRpcChannelRoute *previous = &routes->entries[i];
        if (previous->kind == route.kind && previous->pid == route.pid &&
            !memcmp(previous->token, route.token, sizeof(route.token)) &&
            (previous->channel.key == route.channel.key || route.kind == STYXRPC_ROUTE_PID))
        {
            return previous->channel.key == route.channel.key && previous->unbuffered == route.unbuffered &&
                !strcmp(previous->channel.shape, route.channel.shape);
        }
    }
    if (routes->count == STYXRPC_CHANNEL_SLOTS)
    {
        return 0;
    }
    routes->entries[routes->count++] = route;
    return 1;
}


/* EN: Validate one complete typed wrapper and publish a copied route table on success.
 * RU: Проверить полную типизированную запись и при успехе выдать копию таблицы адресов. */
static StyxRpcStatus read_routes(const char *shape, const void *bytes, size_t size,
    StyxRpcChannelRoutes *routes, int exported)
{
    FVM_SRecord element;
    size_t used;
    RouteList staged = {.exported = exported};
    if (!routes || !bytes || size > STYXRPC_MANAGED_LIMIT ||
        fvm_srecord_read(bytes, size, STYXRPC_MANAGED_LIMIT, &element, &used) != FVM_S_OK || used != size ||
        element.tag != (exported ? STYX_DYNAMIC_REPLY_TAG : STYXRPC_CHANNEL_ELEMENT_TAG))
    {
        return STYXRPC_INVALID;
    }
    element.tag = STYX_MANAGED_GRAPH_TAG;
    if (!styxmanaged_channel_routes(shape, &element, collect, &staged))
    {
        return STYXRPC_INVALID;
    }
    *routes = staged.routes;
    return STYXRPC_OK;
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
StyxRpcStatus styxrpc_channel_element_routes(const char *shape, const void *bytes, size_t size,
    StyxRpcChannelRoutes *routes)
{
    return read_routes(shape, bytes, size, routes, 0);
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
StyxRpcStatus styxrpc_channel_export_routes(const char *shape, const void *bytes, size_t size,
    StyxRpcChannelRoutes *routes)
{
    return read_routes(shape, bytes, size, routes, 1);
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
StyxRpcStatus styxrpc_channel_route_value(const StyxRpcChannelRoute *route, void *bytes, size_t capacity, size_t *used)
{
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    if (!bytes || !styxrpc_route_valid(route))
    {
        return STYXRPC_INVALID;
    }
    unsigned char payload[STYXRPC_CHANNEL_TOKEN_SIZE + 4];
    size_t prefix = 0;
    unsigned long tag = route->unbuffered ? STYX_MANAGED_UNBUFFERED_CHANNEL_TAG : STYX_MANAGED_CHANNEL_TAG;
    if (route->kind == STYXRPC_ROUTE_PID)
    {
        write_word(payload, route->pid);
        prefix = 4;
        tag = STYX_MANAGED_CHANNEL_GRANT_TAG;
    }
    else if (route->kind == STYXRPC_ROUTE_MODULE)
    {
        prefix = sizeof(route->token);
        memcpy(payload, route->token, prefix);
        tag = STYX_MANAGED_CHANNEL_MODULE_GRANT_TAG;
    }
    uint32_t key = route->channel.key | (prefix && route->unbuffered ? REVERSE_BIT : 0);
    write_word(payload + prefix, key);
    return fvm_srecord_write(tag, payload, prefix + 4, bytes, capacity, used) == FVM_S_OK ?
        STYXRPC_OK : STYXRPC_INVALID;
}
