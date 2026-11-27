/* Copyright (c) 2026 bhgv
 * EN: Bind a named Int32 reference to an exact local or remote file behind a byte handle.
 * RU: Привязка именованной ссылки Int32 к точному местному или удалённому файлу через дескриптор.
 */
#include "styxrpc/module_ref_port.h"
#include "styxrpc/named_channel.h"
#include "styxrpc/variable_file.h"
#include "vfs_client_internal.h"
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
static StyxRpcIoOutcome capture(StyxRpcModuleRefPort *port, const unsigned char *command, size_t size, StyxRpcVfsWait *wait)
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
        styxrpc_vfs_int_open_identified_until(port->issuer, ref.path, ref.writable, &file, &ref.node,
            wait->stop ? styxrpc_vfs_control_poll : NULL, wait);
    if (status != STYXRPC_IO_OK)
    {
        styxrpc_file_close(&local_file);
        return local && styxrpc_vfs_control_poll(wait) ? STYXRPC_IO_CANCELLED : status;
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


/* EN: Resolve the immutable descriptor under wait, including after a retired fid.
 * RU: Сверить постоянный описатель с wait, в том числе после закрытия fid. */
static StyxRpcIoOutcome ready(StyxRpcModuleRefPort *port, StyxRpcVfsWait *wait)
{
    if (port->file || port->local.ops)
    {
        return STYXRPC_IO_OK;
    }
    if (port->ref.owner == port->owner)
    {
        return styxrpc_module_ref_open_remote_until(port->issuer, &port->ref, &port->file,
            wait->stop ? styxrpc_vfs_control_poll : NULL, wait);
    }
    StyxRpcIoOutcome status = port->open_local(port->context, &port->ref, &port->local);
    return status != STYXRPC_IO_OK && styxrpc_vfs_control_poll(wait) ? STYXRPC_IO_CANCELLED : status;
}


/* EN: Retire an interrupted remote fid while retaining its original descriptor.
 * Return the transfer status to the caller; the next access rechecks the Qid.
 * RU: Освободить прерванный удалённый fid, сохранив исходный описатель.
 * Вернуть статус передачи. Следующее обращение проверит Qid заново. */
static StyxRpcIoOutcome retire_transfer(StyxRpcModuleRefPort *port, StyxRpcIoOutcome status, StyxRpcVfsWait *wait)
{
    if (status == STYXRPC_IO_CANCELLED || status == STYXRPC_IO_UNCERTAIN)
    {
        styxrpc_vfs_control_close(port->file, wait);
        port->file = NULL;
    }
    return status;
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcIoOutcome styxrpc_module_ref_port_read_until(StyxRpcModuleRefPort *port, uint64_t offset,
    void *bytes, size_t size, size_t *used, StyxRpcVfsStop stop, void *state)
{
    if (!used)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *used = 0;
    if (!port || !port->size || (!bytes && size) ||
        (offset < STYXRPC_REF_PORT_FRAME && (offset || size != STYX_VARIABLE_INT_SIZE)))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    if (styxrpc_vfs_control_poll(&wait))
    {
        return STYXRPC_IO_CANCELLED;
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
    StyxRpcIoOutcome status = ready(port, &wait);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if (port->local.ops)
    {
        unsigned char snapshot[STYX_VARIABLE_INT_SIZE];
        size_t count;
        if (styxrpc_vfs_control_poll(&wait))
        {
            return STYXRPC_IO_CANCELLED;
        }
        if (styxrpc_file_read(&port->local, 0, snapshot, sizeof(snapshot), &count) != STYXRPC_OK || count != size)
        {
            return styxrpc_vfs_control_poll(&wait) ? STYXRPC_IO_CANCELLED : STYXRPC_IO_FAILED;
        }
        memcpy(bytes, snapshot, size);
    }
    else
    {
        status = styxrpc_vfs_int_read_bytes_until(port->file, bytes, stop ? styxrpc_vfs_control_poll : NULL, &wait);
        if (status != STYXRPC_IO_OK)
        {
            return retire_transfer(port, status, &wait);
        }
    }
    *used = size;
    return STYXRPC_IO_OK;
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcIoOutcome styxrpc_module_ref_port_read(StyxRpcModuleRefPort *port, uint64_t offset,
    void *bytes, size_t size, size_t *used)
{
    return styxrpc_module_ref_port_read_until(port, offset, bytes, size, used, NULL, NULL);
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcIoOutcome styxrpc_module_ref_port_write_until(StyxRpcModuleRefPort *port, uint64_t offset,
    const void *bytes, size_t size, StyxRpcVfsStop stop, void *state)
{
    if (!port || !bytes || !size || offset)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    if (styxrpc_vfs_control_poll(&wait))
    {
        return STYXRPC_IO_CANCELLED;
    }
    const unsigned char *command = bytes;
    if (!port->size)
    {
        switch (command[0])
        {
        case STYXRPC_REF_PORT_CAPTURE:
        case STYXRPC_REF_PORT_CAPTURE_LOCAL:
            return capture(port, command, size, &wait);
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
    StyxRpcIoOutcome status = ready(port, &wait);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if (port->local.ops)
    {
        size_t used;
        if (styxrpc_vfs_control_poll(&wait))
        {
            return STYXRPC_IO_CANCELLED;
        }
        if (styxrpc_file_write(&port->local, 0, bytes, size, &used) == STYXRPC_OK && used == size)
        {
            return STYXRPC_IO_OK;
        }
        return styxrpc_vfs_control_poll(&wait) ? STYXRPC_IO_UNCERTAIN : STYXRPC_IO_FAILED;
    }
    status = styxrpc_vfs_int_write_bytes_until(port->file, bytes, stop ? styxrpc_vfs_control_poll : NULL, &wait);
    return retire_transfer(port, status, &wait);
}


/* EN/RU: See module_ref_port.h. / См. module_ref_port.h. */
StyxRpcIoOutcome styxrpc_module_ref_port_write(StyxRpcModuleRefPort *port, uint64_t offset,
    const void *bytes, size_t size)
{
    return styxrpc_module_ref_port_write_until(port, offset, bytes, size, NULL, NULL);
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
