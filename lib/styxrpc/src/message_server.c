/* Copyright (c) 2026 bhgv
 * EN: Publish the shared message implementation as a per-open Styx stream.
 * RU: Публикация общего обработчика сообщений как отдельного потока на каждый fid.
 */
#include "message_internal.h"
#include "styxrpc/vfs_client.h"
#include <stdlib.h>
#include <string.h>

typedef struct ServerMessagePort ServerMessagePort; /* Owned by an open fid. */

struct MessageService
{
    MessageConfig config;      /* Fixed server registrations through destroy. */
    void (*opened)(StyxRpcMessagePort *, void *); /* Optional owner callback. */
    void (*closed)(StyxRpcMessagePort *, void *); /* Optional owner callback. */
    void *context;             /* Borrowed state shared by the hooks. */
    size_t ports;              /* Live fid count on the dispatcher. */
    ServerMessagePort *opened_ports; /* Streams to retire during withdrawal. */
};

struct ServerMessagePort
{
    StyxRpcMessagePort port;   /* Common stream parser and queue. */
    MessageService *service;   /* NULL once withdrawn. */
    ServerMessagePort *next;   /* Next linked open of the same root. */
    uint64_t read_offset;      /* Next position for server-to-client bytes. */
    uint64_t write_offset;     /* Next position for client-to-server bytes. */
};

/* EN: Reject collisions in the ordinary root before copying spec into owned storage.
 * RU: Отклонить коллизии с обычным корнем до копирования spec в своё хранилище. */
MessageService *styxrpc_message_service(const StyxRpcTree *tree, void *context,
    const StyxRpcNode *node, const StyxRpcMessageSpec *spec)
{
    StyxRpcNode existing;
    if (tree->stat(context, node->entry.id, &existing))
    {
        return NULL;
    }
    size_t index;
    for (index = 0; index < STYXRPC_TREE_CHILDREN; ++index)
    {
        StyxRpcEntry entry;
        StyxRpcStatus status = styxrpc_dir_entry(&tree->files, context, 0, index, &entry);
        if (status == STYXRPC_MORE)
        {
            break;
        }
        if (status != STYXRPC_OK || entry.id == node->entry.id ||
            !strcmp(entry.name, node->entry.name))
        {
            return NULL;
        }
    }
    MessageService *service = calloc(1, sizeof(*service));
    if (!service)
    {
        return NULL;
    }
    if (!styxrpc_message_config(&service->config, spec))
    {
        free(service);
        return NULL;
    }
    service->opened = spec->opened;
    service->closed = spec->closed;
    service->context = spec->context;
    return service;
}


/* EN: Allocate a fresh port for an ORDWR open of node. Publish its cookie on success.
 * RU: Выделить новый порт при открытии node на чтение и запись. Выдать cookie при успехе. */
static int open_messages(void *context, uint64_t node, unsigned access, void **cookie)
{
    MessageService *service = context;
    if (!node || access != (STYXRPC_READ | STYXRPC_WRITE) ||
        service->ports == STYXRPC_MESSAGES_PORTS_MAX)
    {
        return 0;
    }
    ServerMessagePort *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return 0;
    }
    if (!styxrpc_message_port_init(&opened->port, &service->config))
    {
        free(opened);
        return 0;
    }
    opened->service = service;
    if (service->opened)
    {
        service->opened(&opened->port, service->context);
    }
    if (opened->port.failed)
    {
        free(opened);
        return 0;
    }
    ++service->ports;
    opened->next = service->opened_ports;
    service->opened_ports = opened;
    *cookie = opened;
    return 1;
}


/* EN: Drain at most capacity at the next read offset on cookie. Empty means idle.
 * RU: Прочитать до capacity со следующего смещения cookie. Ноль означает ожидание. */
static int read_messages(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    ServerMessagePort *opened = cookie;
    StyxRpcMessagePort *port = &opened->port;
    if (port->failed || offset != opened->read_offset)
    {
        return 0;
    }
    size_t count = capacity < port->output_size ? capacity : port->output_size;
    if (count > STYXRPC_VFS_OFFSET_MAX - offset)
    {
        return 0;
    }
    if (count)
    {
        memcpy(bytes, port->output, count);
        styxrpc_message_drain(port, count);
        opened->read_offset += count;
    }
    *size = count;
    return 1;
}


/* EN: Accept contiguous bytes at offset and dispatch checked frames. Report count only on success.
 * RU: Принять смежные байты со смещения offset и доставить проверенные записи. Выдать count при успехе. */
static int write_messages(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    ServerMessagePort *opened = cookie;
    int delivered = 0;
    if (opened->port.failed || offset != opened->write_offset || count > STYXRPC_VFS_OFFSET_MAX - offset ||
        !styxrpc_message_feed(&opened->port, bytes, count, &delivered))
    {
        return 0;
    }
    opened->write_offset += count;
    *size = count;
    return 1;
}


/* EN: Unlink opened, invalidate its borrowed state, then notify its owner. Retain the fid.
 * RU: Удалить opened из списка, отозвать заимствованное состояние и известить владельца. */
static void retire_port(ServerMessagePort *opened)
{
    MessageService *service = opened->service;
    ServerMessagePort **link = &service->opened_ports;
    while (*link != opened)
    {
        link = &(*link)->next;
    }
    *link = opened->next;
    --service->ports;
    opened->service = NULL;
    opened->next = NULL;
    opened->port.failed = 1;
    opened->port.config = NULL;
    if (service->closed)
    {
        service->closed(&opened->port, service->context);
    }
}


/* EN/RU: See message_internal.h. / См. message_internal.h. */
void styxrpc_message_service_destroy(MessageService *service)
{
    if (service)
    {
        while (service->opened_ports)
        {
            retire_port(service->opened_ports);
        }
        free(service);
    }
}


/* EN: Release the fid's storage, notifying the service only on its first retirement.
 * RU: Освободить память fid, известив сервис при первом отзыве. */
static void close_messages(void *cookie)
{
    ServerMessagePort *opened = cookie;
    if (opened->service)
    {
        retire_port(opened);
    }
    free(opened);
}

const StyxRpcVfs styxrpc_message_files = {.open = open_messages, .read = read_messages,
    .write = write_messages, .close = close_messages}; /* Each open owns one duplex stream. */
