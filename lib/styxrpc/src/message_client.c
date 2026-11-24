/* Copyright (c) 2026 bhgv
 * EN: Drive a custom message fid on an existing hosted Styx client.
 * RU: Обмен пользовательскими сообщениями через fid в существующей сессии Styx.
 */
#include "message_internal.h"
#include "styxrpc/vfs_client.h"
#include <stdlib.h>

#define MESSAGE_READ_BATCH 4096U /* Byte budget per pump, before negotiated Styx fragmentation. */

typedef struct ClientMessagePort
{
    StyxRpcMessagePort port; /* First member permits a checked conversion from the public handle. */
    MessageConfig config;    /* Client registrations are owned by this port. */
    StyxRpcVfsFile *file;    /* The single held read/write fid. */
    uint64_t read_offset;    /* Next confirmed server-to-client byte offset. */
    uint64_t write_offset;   /* Next confirmed client-to-server byte offset. */
    int pumping;            /* Reject recursive use from a receive callback. */
} ClientMessagePort;


/* EN/RU: See messages.h. / См. messages.h. */
StyxRpcStatus styxrpc_messages_open(struct StyxClient *session, const StyxRpcMessageSpec *spec,
    StyxRpcMessagePort **out)
{
    if (!session || !out)
    {
        return STYXRPC_INVALID;
    }
    ClientMessagePort *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return STYXRPC_INVALID;
    }
    if (!styxrpc_message_config(&opened->config, spec) ||
        !styxrpc_message_port_init(&opened->port, &opened->config) ||
        styxrpc_vfs_open(session, STYXRPC_MESSAGES_FILE, STYXRPC_FILE_RDWR, &opened->file) != STYXRPC_IO_OK)
    {
        free(opened);
        return STYXRPC_INVALID;
    }
    opened->port.client = 1;
    *out = &opened->port;
    return STYXRPC_OK;
}


/* EN: Transfer queued output and one batch of input on opened, without retrying errors.
 * RU: Передать очередь и принять одну порцию через opened, без повтора ошибок. */
static StyxRpcStatus pump_port(ClientMessagePort *opened)
{
    StyxRpcMessagePort *port = &opened->port;
    if (port->output_size)
    {
        size_t sent;
        if (styxrpc_vfs_file_write(opened->file, opened->write_offset, port->output,
            port->output_size, &sent) != STYXRPC_IO_OK || sent != port->output_size)
        {
            return STYXRPC_INVALID;
        }
        opened->write_offset += sent;
        styxrpc_message_drain(port, sent);
    }
    unsigned char bytes[MESSAGE_READ_BATCH];
    size_t got;
    int delivered = 0;
    if (styxrpc_vfs_file_read(opened->file, opened->read_offset, bytes, sizeof(bytes), &got) != STYXRPC_IO_OK ||
        !styxrpc_message_feed(port, bytes, got, &delivered))
    {
        return STYXRPC_INVALID;
    }
    opened->read_offset += got;
    return delivered ? STYXRPC_OK : STYXRPC_MORE;
}


/* EN/RU: See messages.h. / См. messages.h. */
StyxRpcStatus styxrpc_messages_pump(StyxRpcMessagePort *port)
{
    if (!port || !port->client || port->failed)
    {
        return STYXRPC_INVALID;
    }
    ClientMessagePort *opened = (ClientMessagePort *)port;
    if (opened->pumping)
    {
        return STYXRPC_INVALID;
    }
    opened->pumping = 1;
    StyxRpcStatus status = pump_port(opened);
    opened->pumping = 0;
    port->failed = status == STYXRPC_INVALID;
    return status;
}


/* EN/RU: See messages.h. / См. messages.h. */
void styxrpc_messages_close(StyxRpcMessagePort *port)
{
    if (port && port->client)
    {
        ClientMessagePort *opened = (ClientMessagePort *)port;
        if (!opened->pumping)
        {
            styxrpc_vfs_close(opened->file, 0);
            free(opened);
        }
    }
}
