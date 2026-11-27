/* Copyright (c) 2026 bhgv
 * EN: Import native reverse PID grants transactionally on the issuing attachment.
 * RU: Атомарный импорт нативных встречных PID на выдавшем подключении.
 */
#include "channel_route_internal.h"
#include "load_profile.h"
#include "vfs_client_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>


/* EN: Read identity through its EOF and compare only the exact decimal line.
 * Return 1 on a match. Borrow the opened fid and the caller's stop state.
 * RU: Прочитать identity до конца и сравнить точную десятичную строку.
 * Вернуть 1 при совпадении. Заимствовать открытый fid и состояние остановки. */
static int matches_pid(StyxFid *identity, uint32_t pid, StyxRpcVfsWait *wait)
{
    char expected[sizeof("4294967295\n")], bytes[sizeof(expected)];
    size_t length = (size_t)snprintf(expected, sizeof(expected), "%" PRIu32 "\n", pid);
    size_t seen = 0;
    while (seen < sizeof(bytes))
    {
        StyxTransfer read;
        styxclientreaduntil(identity, seen, bytes + seen, sizeof(bytes) - seen,
            wait->stop ? styxrpc_vfs_control_poll : NULL, wait, &read);
        if (!read.success || !read.confirmed || read.uncertain || read.count > sizeof(bytes) - seen)
        {
            return 0;
        }
        if (!read.count)
        {
            return seen == length && !memcmp(bytes, expected, length);
        }
        seen += read.count;
    }
    return 0;
}


/* EN: Confirm pid and its bus on the borrowed reverse mount. Retire all
 * temporary fids before returning, deferring cleanup if wait stops.
 * RU: Подтвердить pid и его шину на заимствованном встречном подключении.
 * Освободить временные fid до возврата или отложить очистку при остановке wait. */
static StyxRpcStatus check_pid(StyxClient *reverse, uint32_t pid, StyxRpcVfsWait *wait)
{
    char path[sizeof(".prog/4294967295")];
    StyxFid *dir = NULL, *identity = NULL, *bus = NULL;
    snprintf(path, sizeof(path), ".prog/%" PRIu32, pid);
    int accepted = styxrpc_vfs_control_walk(reverse, path, wait, &dir) == STYXRPC_IO_OK &&
        styxrpc_vfs_control_walk_from(dir, ".pid", wait, &identity) == STYXRPC_IO_OK &&
        styxrpc_vfs_control_open(identity, STYXRPC_FILE_READ, wait) == STYXRPC_IO_OK &&
        matches_pid(identity, pid, wait) &&
        styxrpc_vfs_control_walk_from(dir, ".unibus", wait, &bus) == STYXRPC_IO_OK;
    styxrpc_vfs_control_close(bus, wait);
    styxrpc_vfs_control_close(identity, wait);
    styxrpc_vfs_control_close(dir, wait);
    if (styxrpc_vfs_control_poll(wait))
    {
        return STYXRPC_CANCELLED;
    }
    return accepted ? STYXRPC_OK : STYXRPC_INVALID;
}


/* EN: Merge route into staged, preserving exact PID aliases and existing indices.
 * RU: Добавить route в staged, сохранив алиасы PID и существующие индексы. */
static int append(StyxRpcChannelRoutes *staged, const StyxRpcChannelRoute *route, int root)
{
    for (size_t i = 0; i < staged->count; ++i)
    {
        const StyxRpcChannelRoute *old = staged->entries + i;
        if (old->pid == route->pid && (root || old->channel.key == route->channel.key))
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
StyxRpcStatus styxrpc_channel_pid_import_until(StyxClient *reverse, const char *shape,
    const void *bytes, size_t size, StyxRpcChannelRoutes *routes, StyxRpcCallStop stop, void *state)
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
        if (route->kind != STYXRPC_ROUTE_PID || strpbrk(route->channel.shape, "rh") || !append(&staged, route, 1))
        {
            return STYXRPC_INVALID;
        }
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    for (size_t i = 0; i < received.count; ++i)
    {
        StyxRpcStatus status = check_pid(reverse, received.entries[i].pid, &wait);
        if (status != STYXRPC_OK)
        {
            return status;
        }
    }
    if (styxrpc_vfs_control_poll(&wait))
    {
        return STYXRPC_CANCELLED;
    }
    *routes = staged;
    return STYXRPC_OK;
}


/* EN/RU: See styxrpc/channel_route.h. / См. styxrpc/channel_route.h. */
StyxRpcStatus styxrpc_channel_pid_import(StyxClient *reverse, const char *shape,
    const void *bytes, size_t size, StyxRpcChannelRoutes *routes)
{
    return styxrpc_channel_pid_import_until(reverse, shape, bytes, size, routes, NULL, NULL);
}


/* EN: Add only ordinary current-bus children from the checked result on pid.
 * RU: Добавить только дочерние ключи текущей шины из проверенного результата на pid. */
static int import_current(StyxRpcChannelRoutes *routes, uint32_t pid,
    const StyxRpcChannelGrant *grants, size_t count, uint32_t key, const StyxRpcChannelResult *result)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (grants[i].key != key)
        {
            continue;
        }
        const char *shape = grants[i].shape;
        if (!strchr(shape, 'c'))
        {
            return 1;
        }
        StyxRpcChannelRoutes received;
        if (styxrpc_channel_element_routes(shape, result->value, result->size, &received) != STYXRPC_OK)
        {
            return 0;
        }
        StyxRpcChannelRoutes staged = *routes;
        for (size_t j = 0; j < received.count; ++j)
        {
            StyxRpcChannelRoute route = received.entries[j];
            if (route.kind != STYXRPC_ROUTE_CURRENT || strpbrk(route.channel.shape, "rh"))
            {
                return 0;
            }
            route.kind = STYXRPC_ROUTE_PID;
            route.pid = pid;
            if (!append(&staged, &route, 0))
            {
                return 0;
            }
        }
        *routes = staged;
        return 1;
    }
    return 0;
}


/* EN/RU: See styxrpc/channel_route.h. / См. styxrpc/channel_route.h. */
StyxRpcStatus styxrpc_channel_pid_exchange_until(StyxClient *reverse, StyxRpcChannelRoutes *routes, size_t anchor,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result,
    StyxRpcCallStop stop, void *state)
{
    if (!routes || routes->count > STYXRPC_CHANNEL_SLOTS || anchor >= routes->count ||
        !selection || selection->policy || !result || routes->entries[anchor].kind != STYXRPC_ROUTE_PID)
    {
        return STYXRPC_INVALID;
    }
    uint32_t pid = routes->entries[anchor].pid;
    StyxRpcChannelGrant grants[STYXRPC_CHANNEL_SLOTS];
    size_t count = 0;
    for (size_t i = 0; i < routes->count; ++i)
    {
        const StyxRpcChannelRoute *route = routes->entries + i;
        if (route->kind == STYXRPC_ROUTE_PID && route->pid == pid)
        {
            grants[count++] = route->channel;
        }
    }
    StyxRpcChannelResult decoded;
    StyxRpcStatus status = styxrpc_channel_exchange_pid_bound(reverse, pid, grants, count, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, &decoded, stop, state);
    if (status == STYXRPC_OK)
    {
        const StyxRpcGuard *guard = decoded.selected ? selection->guards + decoded.selected - 1 : NULL;
        if (guard && guard->receive && !import_current(routes, pid, grants, count, guard->key, &decoded))
        {
            return STYXRPC_INVALID;
        }
        *result = decoded;
    }
    return status;
}


/* EN/RU: See styxrpc/channel_route.h. / См. styxrpc/channel_route.h. */
StyxRpcStatus styxrpc_channel_pid_exchange(StyxClient *reverse, StyxRpcChannelRoutes *routes, size_t anchor,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    return styxrpc_channel_pid_exchange_until(reverse, routes, anchor, selection, timeout_ms, scratch, scratch_size,
        request, request_capacity, reply, reply_capacity, result, NULL, NULL);
}
