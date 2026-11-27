/* Copyright (c) 2026 bhgv
 * EN: Apply a caller's stop hook to existing 9P path and metadata exchanges.
 * RU: Передавать отмену вызывающего в штатные операции 9P над путями и метаданными.
 */
#include "vfs_client_internal.h"


/* EN: Sample the borrowed wait and remember a callback stop separately from an
 * unrelated protocol deadline. Only the synchronous caller accesses wait.
 * RU: Проверить заимствованный wait и отличить флаг колбэка от тайм-аута обмена.
 * wait доступен только синхронному вызывающему. */
static int stop_requested(void *state)
{
    StyxRpcVfsWait *wait = state;
    if (!wait->stopped && wait->stop)
    {
        wait->stopped = wait->stop(wait->state) != 0;
    }
    return wait->stopped;
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_control_begin(StyxRpcVfsWait *wait, int multiplexed, StyxCancel **token)
{
    *token = NULL;
    if (stop_requested(wait))
    {
        return STYXRPC_IO_CANCELLED;
    }
    if (wait->stop && multiplexed)
    {
        *token = styxcancelcreatepoll(stop_requested, wait);
        if (!*token)
        {
            return STYXRPC_IO_FAILED;
        }
    }
    return STYXRPC_IO_OK;
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_control_end(StyxRpcVfsWait *wait, StyxCancel *token,
    const StyxTransfer *result, int mutating, StyxRpcIoOutcome rejected)
{
    styxcancelfree(token);
    if (result->success)
    {
        return STYXRPC_IO_OK;
    }
    if (mutating && result->uncertain)
    {
        return STYXRPC_IO_UNCERTAIN;
    }
    if (result->cancelled && wait->stopped)
    {
        return STYXRPC_IO_CANCELLED;
    }
    return rejected;
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_control_walk(StyxClient *session, const char *path,
    StyxRpcVfsWait *wait, StyxFid **out)
{
    StyxCancel *token;
    StyxRpcIoOutcome status = styxrpc_vfs_control_begin(wait, styxclientcancellable(session), &token);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if (!token)
    {
        *out = styxclientwalk(session, path);
        return *out ? STYXRPC_IO_OK : STYXRPC_IO_NOT_OPEN;
    }
    StyxTransfer result;
    *out = styxclientwalkcancel(session, path, token, &result);
    return styxrpc_vfs_control_end(wait, token, &result, 0, STYXRPC_IO_NOT_OPEN);
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_control_stat(StyxFid *fid, StyxMeta *meta,
    StyxRpcVfsWait *wait, StyxRpcIoOutcome rejected)
{
    StyxCancel *token;
    StyxRpcIoOutcome status = styxrpc_vfs_control_begin(wait, styxclientfidcancellable(fid), &token);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if (!token)
    {
        return styxclientstat(fid, meta) ? STYXRPC_IO_OK : rejected;
    }
    StyxTransfer result;
    styxclientstatcancel(fid, meta, token, &result);
    return styxrpc_vfs_control_end(wait, token, &result, 0, rejected);
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_control_open(StyxFid *fid, StyxRpcFileMode mode, StyxRpcVfsWait *wait)
{
    StyxCancel *token;
    StyxRpcIoOutcome status = styxrpc_vfs_control_begin(wait, styxclientfidcancellable(fid), &token);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if (!token)
    {
        return styxclientopen(fid, mode) ? STYXRPC_IO_OK : STYXRPC_IO_NOT_OPEN;
    }
    StyxTransfer result;
    styxclientopencancel(fid, mode, token, &result);
    return styxrpc_vfs_control_end(wait, token, &result, 0, STYXRPC_IO_NOT_OPEN);
}
