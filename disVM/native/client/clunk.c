/* Copyright (c) 2026 bhgv
 * EN: Consuming cancellable fid cleanup with independent best-effort retirement.
 * RU: Consuming cancellable fid cleanup с independent best-effort retirement.
 */
#include "internal.h"


/* EN/RU: See client.h; schedule cleanup before consuming the fid's client pin.
 * См. client.h; schedule cleanup до поглощения client pin от fid. */
void styxclientclunklater(StyxFid *fid)
{
    if (!fid)
    {
        return;
    }
    StyxClient *client = fid->client;
    pthread_mutex_lock(&client->lock);
    if (!client->failed)
    {
        styxforgetlocked(client, fid->number);
    }
    pthread_mutex_unlock(&client->lock);
    free(fid);
    styxclientfree(client);
}


/* EN/RU: See client.h; original Rclunk wins, otherwise schedule native cleanup once.
 * См. client.h; original Rclunk приоритетен, иначе schedule native cleanup один раз. */
void styxclientclunkcancel(StyxFid *fid, StyxCancel *cancel)
{
    if (!fid)
    {
        return;
    }
    StyxClient *client = fid->client;
    if (!client->mux)
    {
        styxclientclunklater(fid);
        return;
    }
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = Tclunk;
    request.fid = fid->number;
    pthread_mutex_lock(&client->lock);
    int status = styxrpccancel(client, &request, &reply, cancel);
    if (status < 0 && !client->failed)
    {
        styxforgetlocked(client, fid->number);
    }
    pthread_mutex_unlock(&client->lock);
    free(fid);
    styxclientfree(client);
}


/* EN/RU: See client.h. / См. client.h. */
int styxclientremovecancel(StyxFid *fid, StyxCancel *cancel, StyxTransfer *result)
{
    StyxTransfer discarded;
    if (!result)
    {
        result = &discarded;
    }
    styxopclear(result);
    if (!fid)
    {
        return 0;
    }
    StyxClient *client = fid->client;
    if (cancel && !client->mux)
    {
        styxclientclunklater(fid);
        return 0;
    }
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = Tremove;
    request.fid = fid->number;
    pthread_mutex_lock(&client->lock);
    int status = styxrpccancel(client, &request, &reply, cancel);
    styxopresult(client, status, result);
    if (status < 0 && !client->failed)
    {
        styxforgetlocked(client, fid->number);
    }
    pthread_mutex_unlock(&client->lock);
    free(fid);
    styxclientfree(client);
    return result->success;
}


/* EN/RU: See client.h. / См. client.h. */
int styxclientremove(StyxFid *fid, StyxTransfer *result)
{
    return styxclientremovecancel(fid, NULL, result);
}
