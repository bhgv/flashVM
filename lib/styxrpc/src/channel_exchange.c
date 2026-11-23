/* Copyright (c) 2026 bhgv
 * EN: Synchronous reverse Styx adapter for the current Dis call's UniBus.
 * RU: Синхронный адаптер встречного Styx к UniBus текущего вызова Dis.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/channel_call.h"
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


/* EN: Send each confirmed prefix, then collect one reply from the next file offset.
 * RU: Послать подтверждённые фрагменты, затем собрать ответ со следующего смещения. */
static StyxRpcStatus exchange(StyxFid *fid, const void *request, size_t size,
    void *reply, size_t capacity, uint64_t deadline, size_t *received)
{
    size_t offset = 0;
    while (offset < size)
    {
        uint64_t now = now_ms();
        if (!now || now >= deadline)
        {
            return STYXRPC_MORE;
        }
        StyxTransfer wrote;
        styxclientwrite(fid, offset, (const unsigned char *)request + offset, size - offset, &wrote);
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
        styxclientread(fid, offset + total, (unsigned char *)reply + total, capacity - total, &read);
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


/* EN/RU: See channel_call.h. / См. channel_call.h. */
StyxRpcStatus styxrpc_channel_exchange(StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    size_t size, received;
    uint64_t began = now_ms();
    if (!reverse || !call || !call->pid || !reply || !reply_capacity || reply_capacity > STYXRPC_MANAGED_LIMIT || !result || !began ||
        !timeout_ms || timeout_ms > 60000 ||
        styxrpc_channel_request(call->grants, call->count, selection, scratch, scratch_size,
            request, request_capacity, &size) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    char path[sizeof(".prog/4294967295/.unibus")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32 "/.unibus", call->pid);
    StyxFid *fid = styxclientwalk(reverse, path);
    StyxRpcStatus status = fid && styxclientopen(fid, 2) ?
        exchange(fid, request, size, reply, reply_capacity, began + timeout_ms, &received) : STYXRPC_INVALID;
    styxclientclunk(fid);
    return status == STYXRPC_OK ? styxrpc_channel_result(call->grants, call->count,
        selection, reply, received, result) : status;
}
