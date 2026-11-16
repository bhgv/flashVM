/* Copyright (c) 2026 bhgv
 * EN: Native Topen with acknowledgement-preserving cancellation and uncertain-fid fencing.
 * RU: Native Topen с acknowledgement-preserving cancellation и uncertain-fid fencing.
 */
#include "internal.h"


/* EN/RU: See client.h; preserve open-mode ceiling and per-fid negotiated iounit.
 * См. client.h; сохранить mode ceiling и negotiated iounit для fid. */
static int openfid(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result)
{
    styxopclear(result);
    if (!fid || mode < 0 || mode > 2 || fid->opened || fid->poisoned)
    {
        return 0;
    }
    StyxClient *client = fid->client;
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = Topen;
    request.fid = fid->number;
    request.mode = mode;
    pthread_mutex_lock(&client->lock);
    int status = styxrpccancel(client, &request, &reply, cancel);
    styxopresult(client, status, result);
    int success = result->success;
    if (result->uncertain)
    {
        fid->poisoned = 1;
    }
    if (success)
    {
        fid->qid = reply.qid;
        fid->unit = client->msize-IOHDRSZ;
        if (reply.iounit && reply.iounit < fid->unit)
        {
            fid->unit = reply.iounit;
        }
        fid->opened = 1;
        fid->mode = mode;
    }
    pthread_mutex_unlock(&client->lock);
    return success;
}


/* EN/RU: See client.h; legacy API now also fences an unacknowledged sent Topen.
 * См. client.h; legacy API тоже блокирует повтор после sent Topen без acknowledgement. */
int styxclientopen(StyxFid *fid, int mode)
{
    StyxTransfer result;
    return openfid(fid, mode, NULL, &result);
}


/* EN/RU: See client.h; refuse legacy clients or missing result before transport effects.
 * См. client.h; legacy clients/missing result отклонить до transport effects. */
int styxclientopencancel(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result)
{
    if (!result)
    {
        return 0;
    }
    styxopclear(result);
    return fid && fid->client->mux ? openfid(fid, mode, cancel, result) : 0;
}
