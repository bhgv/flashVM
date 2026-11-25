/* Copyright (c) 2026 bhgv
 * EN: Resolve one reference on the exact issuer session.
 * RU: Разрешить ссылку в точной сессии владельца.
 */
#include "styxrpc/variable_ref.h"
#include "client.h"
#include <inttypes.h>
#include <stdio.h>


/* EN/RU: See variable_ref.h. / См. variable_ref.h. */
StyxRpcIoOutcome styxrpc_int_ref_open(struct StyxClient *issuer, const StyxRpcIntRef *ref, StyxRpcVfsFile **out)
{
    if (!issuer || !ref || !out || ref->pid < STYXRPC_INT_REF_FIRST_PID ||
        (ref->writable != 0 && ref->writable != 1))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    char path[sizeof(".prog/4294967295/.var/ref")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32 "/.var/ref", ref->pid);
    StyxRpcVfsFile *file = NULL;
    StyxRpcIoOutcome status = styxrpc_vfs_int_open(issuer, path, ref->writable, &file);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    StyxMeta meta;
    if (!styxclientstat(file, &meta) || (meta.identity >> 32) != ref->pid)
    {
        styxrpc_vfs_close(file, 0);
        return STYXRPC_IO_NOT_OPEN;
    }
    *out = file;
    return STYXRPC_IO_OK;
}
