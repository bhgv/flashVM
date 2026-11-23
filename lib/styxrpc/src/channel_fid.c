/* Copyright (c) 2026 bhgv
 * EN: Parse, schedule and retire one portable UniBus channel request per fid.
 * RU: Приём, планирование и закрытие одного запроса канала UniBus на fid.
 */
#include "styxrpc/channel_fid.h"
#include <string.h>


/* EN/RU: See channel_fid.h. / См. channel_fid.h. */
StyxRpcStatus styxrpc_channel_fid_write(StyxRpcChannelFid *fid, size_t offset, const void *bytes, size_t size)
{
    if (!fid || !fid->admit || !fid->request || !fid->reply ||
        fid->request_capacity > STYXRPC_MANAGED_LIMIT || fid->reply_capacity < STYXRPC_FRAME_SIZE ||
        fid->refused || fid->pending || fid->ready || !bytes || !size || offset != fid->staged || fid->staged > fid->request_capacity ||
        size > fid->request_capacity - fid->staged)
    {
        return STYXRPC_INVALID;
    }
    memcpy(fid->request + offset, bytes, size);
    fid->staged += size;
    FVM_SRecord record;
    size_t used;
    FVM_SResult framing = fvm_srecord_read(fid->request, fid->staged, fid->request_capacity, &record, &used);
    if (framing == FVM_S_MORE && fid->staged < fid->request_capacity)
    {
        return STYXRPC_MORE;
    }
    if (framing != FVM_S_OK || used != fid->staged ||
        styxrpc_channel_read(fid->grants, fid->grant_count, fid->request, fid->staged, &fid->selection) != STYXRPC_OK)
    {
        fid->refused = 1;
        return STYXRPC_INVALID;
    }
    fid->pending = 1;
    if (fid->admit(fid->context, fid) == 1)
    {
        return STYXRPC_OK;
    }
    fid->pending = 0;
    fid->ready = 0;
    fid->refused = 1;
    return STYXRPC_INVALID;
}


/* EN/RU: See channel_fid.h. / См. channel_fid.h. */
StyxRpcStatus styxrpc_channel_fid_finish(StyxRpcChannelFid *fid, unsigned selected, const void *value, size_t size)
{
    if (!fid || !fid->pending)
    {
        return STYXRPC_INVALID;
    }
    size_t used;
    StyxRpcStatus status = styxrpc_channel_reply(fid->grants, fid->grant_count, &fid->selection,
        selected, value, size, fid->reply, fid->reply_capacity, &used);
    if (status == STYXRPC_OK)
    {
        fid->ready = used;
        fid->pending = 0;
    }
    return status;
}


/* EN/RU: See channel_fid.h. / См. channel_fid.h. */
StyxRpcStatus styxrpc_channel_fid_read(const StyxRpcChannelFid *fid, size_t offset,
    void *bytes, size_t capacity, size_t *size)
{
    if (!size)
    {
        return STYXRPC_INVALID;
    }
    *size = 0;
    if (!fid || fid->refused || (!bytes && capacity))
    {
        return STYXRPC_INVALID;
    }
    if (!fid->ready)
    {
        return STYXRPC_MORE;
    }
    if (offset < fid->ready && capacity)
    {
        size_t remaining = fid->ready - offset;
        *size = capacity < remaining ? capacity : remaining;
        memcpy(bytes, fid->reply + offset, *size);
    }
    return STYXRPC_OK;
}


/* EN/RU: See channel_fid.h. / См. channel_fid.h. */
void styxrpc_channel_fid_close(StyxRpcChannelFid *fid)
{
    if (fid && !fid->refused)
    {
        int pending = fid->pending;
        fid->pending = 0;
        fid->refused = 1;
        if (pending && fid->cancel)
        {
            fid->cancel(fid->context, fid);
        }
    }
}
