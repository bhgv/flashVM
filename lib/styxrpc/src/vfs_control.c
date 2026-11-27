/* Copyright (c) 2026 bhgv
 * EN: Apply a caller's stop hook to existing 9P path and metadata exchanges.
 * RU: Передавать отмену вызывающего в штатные операции 9P над путями и метаданными.
 */
#include "vfs_client_internal.h"


/* EN: Sample the borrowed wait and remember a callback stop separately from an
 * unrelated protocol deadline. Only the synchronous caller accesses wait.
 * RU: Проверить заимствованный wait и отличить флаг колбэка от тайм-аута обмена.
 * wait доступен только синхронному вызывающему. */
int styxrpc_vfs_control_poll(void *state)
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
    if (styxrpc_vfs_control_poll(wait))
    {
        return STYXRPC_IO_CANCELLED;
    }
    if (wait->stop && multiplexed)
    {
        *token = styxcancelcreatepoll(styxrpc_vfs_control_poll, wait);
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


/* EN: Walk from the attached root or the borrowed exact base fid. Sample the
 * wait across fragments and return a new unopened fid in out on success.
 * RU: Пройти от корня подключения либо заимствованного base. Проверять
 * wait между фрагментами и при успехе выдать новый неоткрытый fid. */
static StyxRpcIoOutcome walk(StyxClient *session, StyxFid *base, const char *path,
    StyxRpcVfsWait *wait, StyxFid **out)
{
    StyxCancel *token;
    int multiplexed = base ? styxclientfidcancellable(base) : styxclientcancellable(session);
    StyxRpcIoOutcome status = styxrpc_vfs_control_begin(wait, multiplexed, &token);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if (!token)
    {
        *out = base ? styxclientwalkfrom(base, path) : styxclientwalk(session, path);
        return *out ? STYXRPC_IO_OK : STYXRPC_IO_NOT_OPEN;
    }
    StyxTransfer result;
    *out = base ? styxclientwalkfromcancel(base, path, token, &result) :
        styxclientwalkcancel(session, path, token, &result);
    return styxrpc_vfs_control_end(wait, token, &result, 0, STYXRPC_IO_NOT_OPEN);
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_control_walk(StyxClient *session, const char *path,
    StyxRpcVfsWait *wait, StyxFid **out)
{
    return walk(session, NULL, path, wait, out);
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_control_walk_from(StyxFid *base, const char *path,
    StyxRpcVfsWait *wait, StyxFid **out)
{
    return base ? walk(NULL, base, path, wait, out) : STYXRPC_IO_NOT_OPEN;
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


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
void styxrpc_vfs_control_close(StyxFid *fid, StyxRpcVfsWait *wait)
{
    StyxCancel *token;
    if (!fid)
    {
        return;
    }
    if (styxrpc_vfs_control_begin(wait, styxclientfidcancellable(fid), &token) != STYXRPC_IO_OK)
    {
        styxclientclunklater(fid);
    }
    else if (token)
    {
        styxclientclunkcancel(fid, token);
        styxcancelfree(token);
    }
    else
    {
        styxclientclunk(fid);
    }
}
