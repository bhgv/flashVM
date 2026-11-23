/* Copyright (c) 2026 bhgv
 * EN: Named result lease and one-element FIFO on a serialized C dispatcher.
 * RU: Аренда возвращаемого канала и одноэлементный FIFO в диспетчере C.
 */
#define _POSIX_C_SOURCE 200809L
#include "owned_port.h"
#include "styxrpc/graph.h"
#include "load_profile.h"
#include <sys/random.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

const StyxRpcSchema port_schema = {"()", "(c[s])"}; /* Verified by the Limbo compiler. */
static const StyxRpcChannelGrant grant = {1, "(s)"}; /* Opaque token scopes this key. */

typedef struct Operation
{
    DevicePort *port;    /* Owned by server until each open retires. */
    int authenticated;  /* The first write matched the current result lease. */
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]; /* Exact fid's authorization. */
    unsigned char request[192]; /* Complete buffered operation. */
    unsigned char reply[192];   /* Completed selection. */
    StyxRpcChannelFid fid;      /* Shared portable lifecycle. */
    uint64_t deadline;          /* Monotonic deadline for bounded WAIT. */
} Operation;


/* EN/RU: See owned_port.h. / См. owned_port.h. */
int device_port_publish(DevicePort *port, const StyxRpcChannelCall *call,
    void *reply, size_t capacity, size_t *size)
{
    if (port->active || getrandom(port->token, sizeof(port->token), 0) != (ssize_t)sizeof(port->token))
    {
        return 0;
    }
    unsigned char child[48], result[64], updates[12];
    size_t length, result_size, update_size;
    port->active = styxrpc_channel_export_value(port->token, grant.key, 0,
        child, sizeof(child), &length) == STYXRPC_OK &&
        fvm_srecord_write(STYX_DYNAMIC_REPLY_TAG, child, length, result, sizeof(result), &result_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, NULL, 0, updates, sizeof(updates), &update_size) == FVM_S_OK &&
        styxrpc_channel_call_reply(&port_schema, call, result, result_size, updates, update_size,
            reply, capacity, size) == STYXRPC_OK;
    return port->active;
}


/* EN/RU: See owned_port.h. / См. owned_port.h. */
void device_port_revoke(DevicePort *port)
{
    memset(port, 0, sizeof(*port));
}


/* EN: Verify this open's token against the still-live function fid.
 * RU: Сверить токен открытия с ещё действующим fid функции. */
static int current(const Operation *operation)
{
    return operation->authenticated && operation->port->active &&
        !memcmp(operation->token, operation->port->token, sizeof(operation->token));
}


/* EN: Read monotonic milliseconds for the bounded WAIT selection.
 * RU: Прочитать монотонные миллисекунды для ограниченного WAIT. */
static uint64_t now_ms(void)
{
    struct timespec time;
    return !clock_gettime(CLOCK_MONOTONIC, &time) ?
        UINT64_C(1000) * (uint64_t)time.tv_sec + (uint64_t)time.tv_nsec / 1000000 : 0;
}


/* EN: Commit only one ready guard after constructing a complete reply.
 * RU: Выполнить только одну готовую ветку после кодирования полного ответа. */
static int advance(Operation *operation, StyxRpcChannelFid *fid)
{
    DevicePort *port = operation->port;
    for (size_t i = 0; i < fid->selection.count; ++i)
    {
        const StyxRpcGuard *guard = &fid->selection.guards[i];
        if (guard->receive && port->size)
        {
            if (styxrpc_channel_fid_finish(fid, (unsigned)i + 1, port->value, port->size) != STYXRPC_OK)
            {
                return 0;
            }
            port->size = 0;
            return 1;
        }
        if (!guard->receive && !port->size)
        {
            if (guard->size > sizeof(port->value))
            {
                return styxrpc_channel_fid_finish(fid, 255, NULL, 0) == STYXRPC_OK;
            }
            if (styxrpc_channel_fid_finish(fid, (unsigned)i + 1, NULL, 0) != STYXRPC_OK)
            {
                return 0;
            }
            memcpy(port->value, guard->value, guard->size);
            port->size = guard->size;
            return 1;
        }
    }
    int idle = fid->selection.mode == STYXRPC_CHANNEL_TRY ||
        (fid->selection.mode == STYXRPC_CHANNEL_WAIT && now_ms() >= operation->deadline);
    return !idle || styxrpc_channel_fid_finish(fid, 0, NULL, 0) == STYXRPC_OK;
}


/* EN: Poll belongs to the dispatcher, so an operation has no worker to cancel.
 * RU: Опрос выполняет диспетчер, выделенного рабочего потока для отмены нет. */
static int admit(void *context, StyxRpcChannelFid *fid)
{
    Operation *operation = context;
    if (fid->selection.mode == STYXRPC_CHANNEL_WAIT)
    {
        operation->deadline = now_ms() + 40;
    }
    return advance(operation, fid);
}


/* EN: Allocate per-operation buffers; the first write must identify a live lease.
 * RU: Выделить буферы операции. Первая запись должна указать действующую аренду. */
static int open_bus(void *context, unsigned access, void **cookie)
{
    if (access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    Operation *operation = calloc(1, sizeof(*operation));
    if (!operation)
    {
        return 0;
    }
    operation->port = context;
    operation->fid = (StyxRpcChannelFid){.grants = &grant, .grant_count = 1,
        .request = operation->request, .request_capacity = sizeof(operation->request),
        .reply = operation->reply, .reply_capacity = sizeof(operation->reply),
        .admit = admit, .context = operation};
    *cookie = operation;
    return 1;
}


/* EN: Admit the token at offset zero or forward a contiguous operation prefix.
 * RU: Принять токен при смещении ноль либо передать последовательный фрагмент операции. */
static int write_bus(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    Operation *operation = cookie;
    if (!operation->authenticated)
    {
        if (offset || count != sizeof(operation->token) || !operation->port->active ||
            memcmp(bytes, operation->port->token, count))
        {
            return 0;
        }
        memcpy(operation->token, bytes, count);
        operation->authenticated = 1;
    }
    else if (!current(operation) || offset < sizeof(operation->token) ||
        offset - sizeof(operation->token) > SIZE_MAX ||
        styxrpc_channel_fid_write(&operation->fid, (size_t)(offset - sizeof(operation->token)),
            bytes, count) == STYXRPC_INVALID)
    {
        return 0;
    }
    *size = count;
    return 1;
}


/* EN: Poll one operation and read its reply, using offsets relative to the result.
 * RU: Опросить одну операцию и прочитать ответ по смещениям относительно результата. */
static int read_bus(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    Operation *operation = cookie;
    return current(operation) && offset <= SIZE_MAX &&
        (!operation->fid.pending || advance(operation, &operation->fid)) &&
        styxrpc_channel_fid_read(&operation->fid, (size_t)offset, bytes, capacity, size) != STYXRPC_INVALID;
}


/* EN: Discard this exact operation; the lease remains with its forward function.
 * RU: Освободить точную операцию. Аренда принадлежит прямому fid функции. */
static void close_bus(void *cookie)
{
    Operation *operation = cookie;
    styxrpc_channel_fid_close(&operation->fid);
    free(operation);
}


/* EN/RU: See owned_port.h. / См. owned_port.h. */
StyxRpcResource device_port_resource(DevicePort *port)
{
    return (StyxRpcResource){port, open_bus, read_bus, write_bus, close_bus};
}
