/* Copyright (c) 2026 bhgv
 * EN: Single buffered C channel and the fid-owned lease of a fixture module.
 * RU: Один буферизованный канал C и аренда модуля, закреплённая за fid.
 */
#ifndef STYXRPC_EXAMPLE_OWNED_PORT_H
#define STYXRPC_EXAMPLE_OWNED_PORT_H
#include "styxrpc/channel_call.h"
#include "styxrpc/channel_scheduler.h"
#include "styxrpc/program.h"

extern const StyxRpcSchema port_schema; /* port(): chan of string. */
#define PORT_SIGNATURE UINT32_C(0x21cc50c6) /* Compiled Limbo ABI hash. */

typedef struct DevicePort
{
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]; /* Opaque result authorization. */
    unsigned char value[64];                      /* One complete admitted string frame. */
    size_t size;                                  /* Size of the current FIFO slot. */
    StyxRpcChannelQueue queue;                    /* One registered buffered channel. */
    StyxRpcChannelScheduler scheduler;            /* Borrowed by module operation fids. */
    int active;                                   /* Forward function fid still owns this lease. */
} DevicePort;


/* EN: Publish one buffered string channel on the open function's lifetime.
 * RU: Опубликовать буферизованный канал строк на срок жизни fid функции. */
int device_port_publish(DevicePort *port, const StyxRpcChannelCall *call,
    void *reply, size_t capacity, size_t *size);


/* EN: Revoke old operation fids and clear the queue.
 * RU: Отозвать старые fid операций и очистить очередь. */
void device_port_revoke(DevicePort *port);


/* EN: Create a .unibus resource that handles authenticated operation fids.
 * RU: Создать ресурс .unibus с проверкой полномочий для fid операций. */
StyxRpcResource device_port_resource(DevicePort *port);

#endif
