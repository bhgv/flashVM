/* Copyright (c) 2026 bhgv
 * EN: Import native reverse PID grants transactionally on the issuing attachment.
 * RU: Атомарный импорт нативных встречных PID на выдавшем подключении.
 */
#include "styxrpc/channel_route.h"
#include "load_profile.h"
#include "client.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>


/* EN: Confirm the exact decimal identity and bus of pid on reverse.
 * RU: Подтвердить точный десятичный идентификатор и шину pid на reverse. */
static int check_pid(StyxClient *reverse, uint32_t pid)
{
    char path[sizeof(".prog/4294967295")];
    char expected[sizeof("4294967295\n")], bytes[sizeof(expected)];
    snprintf(path, sizeof(path), ".prog/%" PRIu32, pid);
    size_t length = (size_t)snprintf(expected, sizeof(expected), "%" PRIu32 "\n", pid);
    StyxFid *dir = styxclientwalk(reverse, path);
    StyxFid *identity = dir ? styxclientwalkfrom(dir, ".pid") : NULL;
    StyxFid *bus = NULL;
    size_t seen = 0;
    int ok = identity && styxclientopen(identity, 0);
    while (ok && seen < sizeof(bytes))
    {
        StyxTransfer read;
        styxclientread(identity, seen, bytes + seen, sizeof(bytes) - seen, &read);
        ok = read.success && read.confirmed && !read.uncertain && read.count <= sizeof(bytes) - seen;
        if (!ok || !read.count)
        {
            break;
        }
        seen += read.count;
    }
    if (ok && seen == length && !memcmp(bytes, expected, length))
    {
        bus = styxclientwalkfrom(dir, ".unibus");
    }
    ok = bus != NULL;
    styxclientclunk(bus);
    styxclientclunk(identity);
    styxclientclunk(dir);
    return ok;
}


/* EN: Merge route into staged, preserving exact PID aliases and existing indices.
 * RU: Добавить route в staged, сохранив алиасы PID и существующие индексы. */
static int append(StyxRpcChannelRoutes *staged, const StyxRpcChannelRoute *route)
{
    for (size_t i = 0; i < staged->count; ++i)
    {
        const StyxRpcChannelRoute *old = staged->entries + i;
        if (old->pid == route->pid)
        {
            return old->kind == route->kind && old->channel.key == route->channel.key &&
                old->unbuffered == route->unbuffered && !strcmp(old->channel.shape, route->channel.shape);
        }
    }
    if (staged->count == STYXRPC_CHANNEL_SLOTS)
    {
        return 0;
    }
    staged->entries[staged->count++] = *route;
    return 1;
}


/* EN/RU: See styxrpc/channel_route.h. / См. styxrpc/channel_route.h. */
StyxRpcStatus styxrpc_channel_pid_import(StyxClient *reverse, const char *shape,
    const void *bytes, size_t size, StyxRpcChannelRoutes *routes)
{
    StyxRpcChannelRoutes received;
    const char *end = shape && *shape == '(' ? styxshape_end(shape + 1) : NULL;
    if (!reverse || !routes || routes->count > STYXRPC_CHANNEL_SLOTS ||
        !end || *end != ')' || end[1] || !strchr(shape, 'c') || strpbrk(shape, "rh") ||
        styxrpc_channel_element_routes(shape, bytes, size, &received) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcChannelRoutes staged = *routes;
    for (size_t i = 0; i < received.count; ++i)
    {
        const StyxRpcChannelRoute *route = received.entries + i;
        if (route->kind != STYXRPC_ROUTE_PID || strpbrk(route->channel.shape, "crh") || !append(&staged, route))
        {
            return STYXRPC_INVALID;
        }
    }
    for (size_t i = 0; i < received.count; ++i)
    {
        if (!check_pid(reverse, received.entries[i].pid))
        {
            return STYXRPC_INVALID;
        }
    }
    *routes = staged;
    return STYXRPC_OK;
}
