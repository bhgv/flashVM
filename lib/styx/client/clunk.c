/* Copyright (c) 2026 bhgv
 * EN: Consuming cancellable fid cleanup with independent best-effort retirement.
 * RU: Consuming cancellable fid cleanup с independent best-effort retirement.
 */
#include "internal.h"

/* EN/RU: See client.h; schedule cleanup before consuming the fid's client pin.
 * См. client.h; schedule cleanup до поглощения client pin от fid. */


/* [standalone-contract:styxclientclunklater]
 * EN: styxclientclunklater: native client clunk operations and lifetime.
 * RU: styxclientclunklater: нативные client-операции clunk и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:styxclientclunkcancel]
 * EN: styxclientclunkcancel: native client clunk operations and lifetime.
 * RU: styxclientclunkcancel: нативные client-операции clunk и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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
