/* Copyright (c) 2026 bhgv
 * EN: Named result lease and one-element FIFO on a serialized C dispatcher.
 * RU: Аренда возвращаемого канала и одноэлементный FIFO в диспетчере C.
 */
#define _POSIX_C_SOURCE 200809L
#include "owned_port.h"
#include "styxrpc/graph.h"
#include "load_profile.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

const StyxRpcSchema port_schema = {"()", "(c[s])"}; /* Verified by the Limbo compiler. */
static const StyxRpcChannelGrant grant = {1, "(s)"}; /* Opaque token scopes this key. */

typedef struct Operation
{
    DevicePort *port;    /* Owned by server until each open retires. */
    unsigned char request[192]; /* Complete buffered operation. */
    unsigned char reply[192];   /* Completed selection. */
    StyxRpcChannelFid fid;      /* Shared portable lifecycle. */
} Operation;


/* EN: Supply the owner's monotonic clock to the native channel scheduler.
 * RU: Передать часы владельца нативному планировщику каналов. */
static uint64_t now_ms(void *context)
{
    struct timespec time;
    (void)context;
    return !clock_gettime(CLOCK_MONOTONIC, &time) ?
        UINT64_C(1000) * (uint64_t)time.tv_sec + (uint64_t)time.tv_nsec / 1000000 : 0;
}


/* EN: Allocate operation buffers after the shared bus has authenticated the token.
 * RU: Выделить буферы операции после проверки токена на общей шине. */
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
        .admit = styxrpc_scheduler_admit, .cancel = styxrpc_scheduler_cancel,
        .context = &operation->port->scheduler};
    *cookie = operation;
    return 1;
}


/* EN: Feed each confirmed operation fragment to the scheduler-facing fid.
 * RU: Передать каждый подтверждённый фрагмент операции fid планировщика. */
static int write_bus(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    Operation *operation = cookie;
    if (offset > SIZE_MAX ||
        styxrpc_channel_fid_write(&operation->fid, (size_t)offset, bytes, count) == STYXRPC_INVALID)
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
    if (offset > SIZE_MAX)
    {
        return 0;
    }
    styxrpc_scheduler_progress(&operation->port->scheduler);
    return styxrpc_channel_fid_read(&operation->fid, (size_t)offset, bytes, capacity, size) != STYXRPC_INVALID;
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
int device_port_publish(StyxRpcModuleChannels *channels, DevicePort *port, const StyxRpcChannelCall *call,
    void *reply, size_t capacity, size_t *size)
{
    unsigned char child[48], result[64], updates[12];
    size_t length, result_size, update_size;
    port->queue = (StyxRpcChannelQueue){.grant = grant, .bytes = port->value,
        .sizes = &port->size, .capacity = 1, .stride = sizeof(port->value)};
    const StyxRpcResource resource = {port, open_bus, read_bus, write_bus, close_bus};
    if (!styxrpc_scheduler_init(&port->scheduler, &port->queue, 1, now_ms, NULL) ||
        !styxrpc_module_channels_publish(channels, &resource, port->token))
    {
        return 0;
    }
    if (styxrpc_channel_export_value(port->token, grant.key, 0, child, sizeof(child), &length) == STYXRPC_OK &&
        fvm_srecord_write(STYX_DYNAMIC_REPLY_TAG, child, length, result, sizeof(result), &result_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, NULL, 0, updates, sizeof(updates), &update_size) == FVM_S_OK &&
        styxrpc_channel_call_reply(&port_schema, call, result, result_size, updates, update_size,
            reply, capacity, size) == STYXRPC_OK)
    {
        return 1;
    }
    styxrpc_module_channels_revoke(channels, port->token);
    return 0;
}
