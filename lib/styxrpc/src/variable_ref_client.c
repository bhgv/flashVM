/* Copyright (c) 2026 bhgv
 * EN: Resolve one reference on the exact issuer session.
 * RU: Разрешить ссылку в точной сессии владельца.
 */
#include "styxrpc/variable_ref.h"
#include "styxrpc/module_ref.h"
#include "vfs_client_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>


/* EN/RU: See variable_ref.h. / См. variable_ref.h. */
StyxRpcIoOutcome styxrpc_int_ref_open_until(StyxClient *issuer, const StyxRpcIntRef *ref, StyxRpcVfsFile **out,
    StyxRpcVfsStop stop, void *state)
{
    if (!issuer || !ref || !out || ref->pid < STYXRPC_INT_REF_FIRST_PID ||
        (ref->writable != 0 && ref->writable != 1))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    char path[sizeof(".prog/4294967295/.var/ref")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32 "/.var/ref", ref->pid);
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxRpcVfsFile *file = NULL;
    uint64_t identity;
    StyxRpcIoOutcome status = styxrpc_vfs_int_open_identified_until(issuer, path, ref->writable, &file, &identity,
        stop ? styxrpc_vfs_control_poll : NULL, &wait);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if ((identity >> 32) != ref->pid)
    {
        styxrpc_vfs_control_close(file, &wait);
        return STYXRPC_IO_NOT_OPEN;
    }
    *out = file;
    return STYXRPC_IO_OK;
}


/* EN/RU: See variable_ref.h. / См. variable_ref.h. */
StyxRpcIoOutcome styxrpc_int_ref_open(StyxClient *issuer, const StyxRpcIntRef *ref, StyxRpcVfsFile **out)
{
    return styxrpc_int_ref_open_until(issuer, ref, out, NULL, NULL);
}


/* EN/RU: See module_ref.h. / См. module_ref.h. */
StyxRpcIoOutcome styxrpc_module_ref_open_remote_until(StyxClient *issuer, const StyxRpcModuleRef *ref,
    StyxRpcVfsFile **out, StyxRpcVfsStop stop, void *state)
{
    if (!issuer || !ref || !out || !ref->node || !memchr(ref->path, 0, sizeof(ref->path)) ||
        !styxrpc_module_ref_path(ref->path))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxRpcVfsFile *file;
    uint64_t identity;
    StyxRpcIoOutcome status = styxrpc_vfs_int_open_identified_until(issuer, ref->path, ref->writable, &file, &identity,
        stop ? styxrpc_vfs_control_poll : NULL, &wait);
    if (status == STYXRPC_IO_OK)
    {
        if (identity != ref->node)
        {
            styxrpc_vfs_control_close(file, &wait);
            return STYXRPC_IO_NOT_OPEN;
        }
        *out = file;
    }
    return status;
}


/* EN/RU: See module_ref.h. / См. module_ref.h. */
StyxRpcIoOutcome styxrpc_module_ref_open_remote(StyxClient *issuer, const StyxRpcModuleRef *ref,
    StyxRpcVfsFile **out)
{
    return styxrpc_module_ref_open_remote_until(issuer, ref, out, NULL, NULL);
}
