/* Copyright (c) 2026 bhgv
 * EN: Synchronous reverse Styx adapter for the current Dis call's UniBus.
 * RU: Синхронный адаптер встречного Styx к UniBus текущего вызова Dis.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/channel_call.h"
#include "channel_route_internal.h"
#include "client.h"
#include <stdio.h>
#include <inttypes.h>
#include <time.h>


/* EN: Monotonic elapsed-time source in milliseconds, zero on failure.
 * RU: Монотонное время в миллисекундах, ноль при ошибке. */
static uint64_t now_ms(void)
{
    struct timespec now;
    return !clock_gettime(CLOCK_MONOTONIC, &now) ?
        UINT64_C(1000) * (uint64_t)now.tv_sec + (uint64_t)now.tv_nsec / 1000000 : 0;
}


/* EN: Send one framed request on fid and collect its reply before deadline.
 * A module token is a separate first write. Module replies start at offset zero.
 * RU: Послать кадр через fid и собрать ответ до deadline.
 * Токен модуля записывается отдельно. Ответ модуля начинается со смещения ноль. */
static StyxRpcStatus exchange(StyxFid *fid, const unsigned char *token, const void *request, size_t size,
    void *reply, size_t capacity, uint64_t deadline, size_t *received)
{
    size_t prefix = token ? STYXRPC_CHANNEL_TOKEN_SIZE : 0;
    if (token)
    {
        uint64_t now = now_ms();
        if (!now || now >= deadline)
        {
            return STYXRPC_MORE;
        }
        StyxTransfer auth;
        styxclientwrite(fid, 0, token, prefix, &auth);
        if (!auth.success || !auth.confirmed || auth.uncertain || auth.count != prefix)
        {
            return STYXRPC_INVALID;
        }
    }
    size_t offset = 0;
    while (offset < size)
    {
        uint64_t now = now_ms();
        if (!now || now >= deadline)
        {
            return STYXRPC_MORE;
        }
        StyxTransfer wrote;
        styxclientwrite(fid, prefix + offset, (const unsigned char *)request + offset, size - offset, &wrote);
        if (!wrote.success || !wrote.confirmed || wrote.uncertain || !wrote.count || wrote.count > size - offset)
        {
            return STYXRPC_INVALID;
        }
        offset += wrote.count;
    }
    const struct timespec pause = {0, 1000000};
    size_t total = 0;
    while (total < capacity)
    {
        uint64_t now = now_ms();
        if (!now || now >= deadline)
        {
            return STYXRPC_MORE;
        }
        StyxTransfer read;
        styxclientread(fid, (token ? 0 : offset) + total, (unsigned char *)reply + total, capacity - total, &read);
        if (!read.success || !read.confirmed || read.uncertain || read.count > capacity - total)
        {
            return STYXRPC_INVALID;
        }
        if (!read.count)
        {
            nanosleep(&pause, NULL);
            continue;
        }
        total += read.count;
        size_t extent;
        FVM_SRecord record;
        FVM_SResult status = fvm_srecord_read(reply, total, capacity, &record, &extent);
        if (status == FVM_S_OK && extent == total)
        {
            *received = total;
            return STYXRPC_OK;
        }
        if (status != FVM_S_MORE)
        {
            return STYXRPC_INVALID;
        }
    }
    return STYXRPC_INVALID;
}


/* EN: Use one authenticated bus and close its temporary fid before publishing an outcome.
 * RU: Использовать одну шину с проверкой и закрыть временный fid до публикации результата. */
static StyxRpcStatus exchange_on(StyxClient *session, const char *path, StyxFid *module, const unsigned char *token,
    const StyxRpcChannelGrant *grants, size_t grant_count, const StyxRpcSelection *selection,
    unsigned timeout_ms, void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    size_t size, received;
    uint64_t began = now_ms();
    if ((!module && (!session || !path)) || !reply || !reply_capacity || reply_capacity > STYXRPC_MANAGED_LIMIT ||
        !result || !began || !timeout_ms || timeout_ms > 60000 ||
        styxrpc_channel_request(grants, grant_count, selection, scratch, scratch_size,
            request, request_capacity, &size) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    StyxFid *fid = module ? styxclientwalkfrom(module, ".unibus") : styxclientwalk(session, path);
    StyxRpcStatus status = fid && styxclientopen(fid, 2) ?
        exchange(fid, token, request, size, reply, reply_capacity, began + timeout_ms, &received) : STYXRPC_INVALID;
    styxclientclunk(fid);
    return status == STYXRPC_OK ? styxrpc_channel_result(grants, grant_count,
        selection, reply, received, result) : status;
}


/* EN/RU: See channel_call.h. / См. channel_call.h. */
StyxRpcStatus styxrpc_channel_exchange(StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    if (!call || !call->pid)
    {
        return STYXRPC_INVALID;
    }
    char path[sizeof(".prog/4294967295/.unibus")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32 "/.unibus", call->pid);
    return exchange_on(reverse, path, NULL, NULL, call->grants, call->count, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, result);
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
StyxRpcStatus styxrpc_channel_route_exchange(StyxClient *session, const char *module_bus,
    const StyxRpcChannelRoute *route, const StyxRpcSelection *selection, unsigned timeout_ms,
    void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    if (!styxrpc_route_valid(route) || route->kind == STYXRPC_ROUTE_CURRENT)
    {
        return STYXRPC_INVALID;
    }
    char path[sizeof(".prog/4294967295/.unibus")];
    const unsigned char *token = NULL;
    const char *endpoint = module_bus;
    if (route->kind == STYXRPC_ROUTE_PID)
    {
        if (module_bus)
        {
            return STYXRPC_INVALID;
        }
        snprintf(path, sizeof(path), ".prog/%" PRIu32 "/.unibus", route->pid);
        endpoint = path;
    }
    else if (!module_bus || !*module_bus)
    {
        return STYXRPC_INVALID;
    }
    else
    {
        token = route->token;
    }
    return exchange_on(session, endpoint, NULL, token, &route->channel, 1, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, result);
}


/* EN/RU: See channel_route_internal.h. / См. channel_route_internal.h. */
StyxRpcStatus styxrpc_channel_exchange_bound(StyxFid *module, const unsigned char *token,
    const StyxRpcChannelGrant *grants, size_t count, const StyxRpcSelection *selection,
    unsigned timeout_ms, void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    return exchange_on(NULL, NULL, module, token, grants, count, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, result);
}
