/* Copyright (c) 2026 bhgv
 * EN: Bind a named Int32 reference to an exact local or remote file behind a byte handle.
 * RU: Привязка именованной ссылки Int32 к точному местному или удалённому файлу через дескриптор.
 */
#include "styxrpc/module_ref_port.h"
#include "styxrpc/named_channel.h"
#include "styxrpc/variable_file.h"
#include "client.h"
#include <stdlib.h>
#include <string.h>


struct StyxRpcModuleRefPort
{
    struct StyxClient *issuer;                    /* Owned pin used for capture or the eventual open. */
    unsigned owner;                               /* Role of this session's remote publication. */
    StyxRpcModuleRefLocalOpen open_local;           /* Optional opposite-role resolver. */
    StyxRpcModuleRefLocalCapture capture_local;     /* Optional opposite-role capture and Qid issuer. */
    void *context;                                 /* Borrowed local resolver state. */
    unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE]; /* Pair nonce, copied at open. */
    StyxRpcModuleRef ref;                          /* Valid after either bind command succeeds. */
    StyxRpcVfsFile *file;                          /* Owned exact fid once captured or resolved. */
    StyxRpcFile local;                             /* Owned exact local file after resolving a local ref. */
    size_t size;                                   /* Tagged descriptor extent, zero before bind. */
    unsigned char frame[1U + STYXRPC_MODULE_REF_WIRE_MAX]; /* Cached byte-identical reference. */
};


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcModuleRefPort *styxrpc_module_ref_port_open(struct StyxClient *issuer, unsigned owner, const void *scope)
{
    return styxrpc_module_ref_port_open_paired(issuer, owner, scope, NULL, NULL);
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcModuleRefPort *styxrpc_module_ref_port_open_paired(struct StyxClient *issuer, unsigned owner,
    const void *scope, StyxRpcModuleRefLocalOpen local, void *context)
{
    return styxrpc_module_ref_port_open_owned(issuer, owner, scope, local, NULL, context);
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcModuleRefPort *styxrpc_module_ref_port_open_owned(struct StyxClient *issuer, unsigned owner,
    const void *scope, StyxRpcModuleRefLocalOpen local, StyxRpcModuleRefLocalCapture capture, void *context)
{
    if (!issuer || !scope || owner > STYXRPC_MODULE_REF_REVERSE || (capture && !local))
    {
        return NULL;
    }
    StyxRpcModuleRefPort *port = calloc(1, sizeof(*port));
    if (!port || !styxclientretain(issuer))
    {
        free(port);
        return NULL;
    }
    port->issuer = issuer;
    port->owner = owner;
    port->open_local = local;
    port->capture_local = capture;
    port->context = context;
    memcpy(port->scope, scope, sizeof(port->scope));
    return port;
}


/* EN: Open the owner selected by command[0], publishing identity only after encoding.
 * RU: Открыть владельца, выбранного command[0], и выдать идентичность после кодирования. */
static StyxRpcIoOutcome capture(StyxRpcModuleRefPort *port, const unsigned char *command, size_t size)
{
    StyxRpcModuleRef ref = {0};
    if (size <= 2 || size - 2 > sizeof(ref.path) - 1 || command[1] > 1 ||
        memchr(command + 2, 0, size - 2))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    memcpy(ref.path, command + 2, size - 2);
    if (!styxrpc_module_ref_path(ref.path))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    ref.writable = command[1];
    int local = command[0] == STYXRPC_REF_PORT_CAPTURE_LOCAL;
    if (local && !port->capture_local)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    ref.owner = local ? 1U - port->owner : port->owner;
    StyxRpcVfsFile *file = NULL;
    StyxRpcFile local_file = {0};
    StyxRpcIoOutcome status = local ? port->capture_local(port->context, &ref, &local_file) :
        styxrpc_vfs_int_open_identified(port->issuer, ref.path, ref.writable, &file, &ref.node);
    if (status != STYXRPC_IO_OK)
    {
        styxrpc_file_close(&local_file);
        return status;
    }
    size_t length = styxrpc_module_ref_encode(&ref, port->scope, port->frame + 1, sizeof(port->frame) - 1);
    if (!length)
    {
        styxrpc_vfs_close(file, 1);
        styxrpc_file_close(&local_file);
        return STYXRPC_IO_FAILED;
    }
    port->frame[0] = STYXRPC_NAMED_MODULE_REF;
    port->size = length + 1;
    port->ref = ref;
    port->file = file;
    port->local = local_file;
    return STYXRPC_IO_OK;
}


/* EN: Copy one frame for an admitted owner, without any ordinary VFS request.
 * RU: Скопировать кадр допустимого владельца без обращения к обычной VFS. */
static StyxRpcIoOutcome adopt(StyxRpcModuleRefPort *port, const unsigned char *frame, size_t size)
{
    StyxRpcModuleRef ref;
    if (!size || size > sizeof(port->frame) || frame[0] != STYXRPC_NAMED_MODULE_REF ||
        !styxrpc_module_ref_decode(frame + 1, size - 1, port->scope, &ref) ||
        (ref.owner != port->owner && !port->open_local))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    memcpy(port->frame, frame, size);
    port->size = size;
    port->ref = ref;
    return STYXRPC_IO_OK;
}


/* EN: Resolve the immutable identity on first value access and retain the exact opened fid.
 * RU: При первом обращении сверить постоянную идентичность и удержать открытый fid. */
static StyxRpcIoOutcome ready(StyxRpcModuleRefPort *port)
{
    if (port->file || port->local.ops)
    {
        return STYXRPC_IO_OK;
    }
    return port->ref.owner == port->owner ?
        styxrpc_module_ref_open_remote(port->issuer, &port->ref, &port->file) :
        port->open_local(port->context, &port->ref, &port->local);
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcIoOutcome styxrpc_module_ref_port_read(StyxRpcModuleRefPort *port, uint64_t offset,
    void *bytes, size_t size, size_t *used)
{
    if (!used)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *used = 0;
    if (!port || !port->size || (!bytes && size))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    if (offset >= STYXRPC_REF_PORT_FRAME)
    {
        uint64_t at = offset - STYXRPC_REF_PORT_FRAME;
        size_t left = at < port->size ? port->size - (size_t)at : 0;
        *used = size < left ? size : left;
        if (*used)
        {
            memcpy(bytes, port->frame + (size_t)at, *used);
        }
        return STYXRPC_IO_OK;
    }
    if (offset || size != STYX_VARIABLE_INT_SIZE)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = ready(port);
    if (status == STYXRPC_IO_OK)
    {
        if (port->local.ops)
        {
            status = styxrpc_file_read(&port->local, 0, bytes, size, used) == STYXRPC_OK ?
                STYXRPC_IO_OK : STYXRPC_IO_FAILED;
        }
        else
        {
            status = styxrpc_vfs_file_read(port->file, 0, bytes, size, used);
        }
        if (status == STYXRPC_IO_OK && *used != size)
        {
            status = STYXRPC_IO_FAILED;
        }
    }
    return status;
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcIoOutcome styxrpc_module_ref_port_write(StyxRpcModuleRefPort *port, uint64_t offset,
    const void *bytes, size_t size)
{
    if (!port || !bytes || !size || offset)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    const unsigned char *command = bytes;
    if (!port->size)
    {
        switch (command[0])
        {
        case STYXRPC_REF_PORT_CAPTURE:
        case STYXRPC_REF_PORT_CAPTURE_LOCAL:
            return capture(port, command, size);
        case STYXRPC_REF_PORT_ADOPT:
            return adopt(port, command + 1, size - 1);
        default:
            return STYXRPC_IO_NOT_OPEN;
        }
    }
    if (!port->ref.writable || size != STYX_VARIABLE_INT_SIZE)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = ready(port);
    if (status == STYXRPC_IO_OK)
    {
        size_t used;
        if (port->local.ops)
        {
            status = styxrpc_file_write(&port->local, 0, bytes, size, &used) == STYXRPC_OK ?
                STYXRPC_IO_OK : STYXRPC_IO_FAILED;
        }
        else
        {
            status = styxrpc_vfs_file_write(port->file, 0, bytes, size, &used);
        }
        if (status == STYXRPC_IO_OK && used != size)
        {
            status = STYXRPC_IO_FAILED;
        }
    }
    return status;
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
void styxrpc_module_ref_port_close(StyxRpcModuleRefPort *port)
{
    if (port)
    {
        styxrpc_vfs_close(port->file, 1);
        styxrpc_file_close(&port->local);
        styxclientfree(port->issuer);
        free(port);
    }
}
