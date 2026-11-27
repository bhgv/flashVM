/* Copyright (c) 2026 bhgv
 * EN: Synchronous reverse Styx adapter for the current Dis call's UniBus.
 * RU: Синхронный адаптер встречного Styx к UniBus текущего вызова Dis.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/channel_call.h"
#include "channel_route_internal.h"
#include "vfs_client_internal.h"
#include <stdio.h>
#include <inttypes.h>
#include <time.h>

#define CHANNEL_REPLY_POLL_NS 1000000L /* Pause between empty reads. */
#define CHANNEL_TIMEOUT_MAX_MS 60000U /* Maximum caller-specified exchange budget. */


/* EN: Monotonic elapsed-time source in milliseconds, zero on failure.
 * RU: Монотонное время в миллисекундах, ноль при ошибке. */
static uint64_t now_ms(void)
{
    struct timespec now;
    return !clock_gettime(CLOCK_MONOTONIC, &now) ?
        UINT64_C(1000) * (uint64_t)now.tv_sec + (uint64_t)now.tv_nsec / 1000000 : 0;
}


/* EN: Require a confirmed complete token write on fid; sample optional stop(state).
 * Cancellation here precedes submission of any channel guard.
 * RU: Требовать подтверждённой записи токена на fid; проверять необязательный
 * stop(state). При отмене ни одно условие выбора канала ещё не отправлено. */
static StyxRpcStatus authenticate(StyxFid *fid, const unsigned char *token, StyxRpcCallStop stop, void *state)
{
    StyxTransfer auth;
    styxclientwriteuntil(fid, 0, token, STYXRPC_CHANNEL_TOKEN_SIZE, stop, state, &auth);
    if (stop && auth.cancelled && !auth.success)
    {
        return STYXRPC_CANCELLED;
    }
    return auth.success && auth.confirmed && !auth.uncertain &&
        auth.count == STYXRPC_CHANNEL_TOKEN_SIZE ? STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See channel_route_internal.h. / См. channel_route_internal.h. */
StyxRpcStatus styxrpc_channel_authenticate_bound(StyxFid *module, const unsigned char *token, StyxRpcVfsWait *wait)
{
    StyxFid *fid = NULL;
    int accepted = styxrpc_vfs_control_walk_from(module, ".unibus", wait, &fid) == STYXRPC_IO_OK &&
        styxrpc_vfs_control_open(fid, STYXRPC_FILE_RDWR, wait) == STYXRPC_IO_OK &&
        authenticate(fid, token, wait->stop ? styxrpc_vfs_control_poll : NULL, wait) == STYXRPC_OK;
    styxrpc_vfs_control_close(fid, wait);
    if (styxrpc_vfs_control_poll(wait))
    {
        return STYXRPC_CANCELLED;
    }
    return accepted ? STYXRPC_OK : STYXRPC_INVALID;
}


/* EN: Send one framed request on fid and collect its reply before deadline.
 * A module token is a separate first write. Module replies start at offset zero.
 * RU: Послать кадр через fid и собрать ответ до deadline.
 * Токен модуля записывается отдельно. Ответ модуля начинается со смещения ноль. */
static StyxRpcStatus exchange(StyxFid *fid, const unsigned char *token, const void *request, size_t size,
    void *reply, size_t capacity, uint64_t deadline, size_t *received, StyxRpcCallStop stop, void *state)
{
    size_t prefix = token ? STYXRPC_CHANNEL_TOKEN_SIZE : 0;
    if (token)
    {
        uint64_t now = now_ms();
        if (!now || now >= deadline)
        {
            return STYXRPC_MORE;
        }
        StyxRpcStatus status = authenticate(fid, token, stop, state);
        if (status != STYXRPC_OK)
        {
            return status;
        }
        if (stop && stop(state))
        {
            return STYXRPC_CANCELLED;
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
        styxclientwriteuntil(fid, prefix + offset, (const unsigned char *)request + offset, size - offset,
            stop, state, &wrote);
        if (stop && wrote.cancelled && !wrote.success)
        {
            return offset || wrote.uncertain ? STYXRPC_CANCELLED_UNCERTAIN : STYXRPC_CANCELLED;
        }
        if (!wrote.success || !wrote.confirmed || wrote.uncertain || !wrote.count || wrote.count > size - offset)
        {
            return STYXRPC_INVALID;
        }
        offset += wrote.count;
    }
    const struct timespec pause = {0, CHANNEL_REPLY_POLL_NS};
    size_t total = 0;
    while (total < capacity)
    {
        uint64_t now = now_ms();
        if (!now || now >= deadline)
        {
            return STYXRPC_MORE;
        }
        StyxTransfer read;
        styxclientreaduntil(fid, (token ? 0 : offset) + total, (unsigned char *)reply + total, capacity - total,
            stop, state, &read);
        if (stop && read.cancelled && !read.success)
        {
            return STYXRPC_CANCELLED_UNCERTAIN;
        }
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
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result, StyxRpcCallStop stop, void *state)
{
    size_t size, received;
    uint64_t began = now_ms();
    if ((!module && (!session || !path)) || !reply || !reply_capacity || reply_capacity > STYXRPC_MANAGED_LIMIT ||
        !result || !began || !timeout_ms || timeout_ms > CHANNEL_TIMEOUT_MAX_MS ||
        styxrpc_channel_request(grants, grant_count, selection, scratch, scratch_size,
            request, request_capacity, &size) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxFid *fid = NULL;
    StyxRpcIoOutcome walked = module ? styxrpc_vfs_control_walk_from(module, ".unibus", &wait, &fid) :
        styxrpc_vfs_control_walk(session, path, &wait, &fid);
    StyxRpcStatus status;
    if (walked == STYXRPC_IO_OK && styxrpc_vfs_control_open(fid, STYXRPC_FILE_RDWR, &wait) == STYXRPC_IO_OK)
    {
        status = styxrpc_vfs_control_poll(&wait) ? STYXRPC_CANCELLED :
            exchange(fid, token, request, size, reply, reply_capacity, began + timeout_ms, &received,
                stop ? styxrpc_vfs_control_poll : NULL, &wait);
    }
    else
    {
        status = wait.stopped ? STYXRPC_CANCELLED : STYXRPC_INVALID;
    }
    if (status == STYXRPC_CANCELLED)
    {
        /* EN: Before a guard write, cleanup can retire only the temporary bus fid.
         * RU: До записи ветви очистке подлежит только временный fid шины. */
        styxrpc_vfs_control_close(fid, &wait);
    }
    else
    {
        styxclientclunk(fid);
    }
    return status == STYXRPC_OK ? styxrpc_channel_result(grants, grant_count,
        selection, reply, received, result) : status;
}


/* EN/RU: See channel_route_internal.h. / См. channel_route_internal.h. */
StyxRpcStatus styxrpc_channel_exchange_pid_bound(StyxClient *reverse, uint32_t pid,
    const StyxRpcChannelGrant *grants, size_t count,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result,
    StyxRpcCallStop stop, void *state)
{
    if (!pid)
    {
        return STYXRPC_INVALID;
    }
    char path[sizeof(".prog/4294967295/.unibus")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32 "/.unibus", pid);
    return exchange_on(reverse, path, NULL, NULL, grants, count, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, result, stop, state);
}


/* EN/RU: See channel_call.h. / См. channel_call.h. */
StyxRpcStatus styxrpc_channel_exchange_until(StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result,
    StyxRpcCallStop stop, void *state)
{
    return call ? styxrpc_channel_exchange_pid_bound(reverse, call->pid, call->grants, call->count, selection,
        timeout_ms, scratch, scratch_size, request, request_capacity, reply, reply_capacity,
        result, stop, state) : STYXRPC_INVALID;
}


/* EN/RU: See channel_call.h. / См. channel_call.h. */
StyxRpcStatus styxrpc_channel_exchange(StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    return styxrpc_channel_exchange_until(reverse, call, selection, timeout_ms, scratch, scratch_size,
        request, request_capacity, reply, reply_capacity, result, NULL, NULL);
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
StyxRpcStatus styxrpc_channel_route_exchange_until(StyxClient *session, const char *module_bus,
    const StyxRpcChannelRoute *route, const StyxRpcSelection *selection, unsigned timeout_ms,
    void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result, StyxRpcCallStop stop, void *state)
{
    if (!styxrpc_route_valid(route) || route->kind == STYXRPC_ROUTE_CURRENT)
    {
        return STYXRPC_INVALID;
    }
    if (route->kind == STYXRPC_ROUTE_PID)
    {
        return module_bus ? STYXRPC_INVALID : styxrpc_channel_exchange_pid_bound(session, route->pid, &route->channel,
            1, selection, timeout_ms, scratch, scratch_size, request, request_capacity, reply, reply_capacity,
            result, stop, state);
    }
    if (!module_bus || !*module_bus)
    {
        return STYXRPC_INVALID;
    }
    return exchange_on(session, module_bus, NULL, route->token, &route->channel, 1, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, result, stop, state);
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
StyxRpcStatus styxrpc_channel_route_exchange(StyxClient *session, const char *module_bus,
    const StyxRpcChannelRoute *route, const StyxRpcSelection *selection, unsigned timeout_ms,
    void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    return styxrpc_channel_route_exchange_until(session, module_bus, route, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, result, NULL, NULL);
}


/* EN/RU: See channel_route_internal.h. / См. channel_route_internal.h. */
StyxRpcStatus styxrpc_channel_exchange_bound(StyxFid *module, const unsigned char *token,
    const StyxRpcChannelGrant *grants, size_t count, const StyxRpcSelection *selection,
    unsigned timeout_ms, void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result, StyxRpcCallStop stop, void *state)
{
    return exchange_on(NULL, NULL, module, token, grants, count, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, result, stop, state);
}
