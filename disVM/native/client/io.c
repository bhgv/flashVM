/* Copyright (c) 2026 bhgv
 * EN: Precise remote chunk acknowledgements, mode ceilings and uncertain-effect poison.
 * RU: Точные remote chunk acknowledgements, mode ceilings и uncertain-effect poison.
 */
#include "internal.h"


/* EN/RU: One native data operation under connection lock; retain exact Rread/Rwrite count.
 * Одна native data-операция под lock; сохранить точный Rread/Rwrite count. */
static void chunk(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, int writing, StyxTransfer *result)
{
    if (!result)
    {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->confirmed = 1;
    if (!fid || !fid->opened || fid->poisoned || offset > 0x7fffffffffffffffULL || (!bytes && size) ||
        (writing ? fid->mode == 0 : fid->mode == 1))
    {
        return;
    }
    if (!size)
    {
        result->success = 1;
        return;
    }
    StyxClient *client = fid->client;
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = writing ? Twrite : Tread;
    request.fid = fid->number;
    request.offset = offset;
    request.count = size > fid->unit ? fid->unit : size;
    request.data = bytes;
    pthread_mutex_lock(&client->lock);
    int status = styxrpc(client, &request, &reply);
    if (status == 1 && reply.count > request.count)
    {
        client->failed = 1;
        status = -1;
    }
    if (status == 1)
    {
        result->count = reply.count;
        result->success = 1;
        if (!writing && reply.count)
        {
            memcpy(bytes, reply.data, reply.count);
        }
    }
    else if (client->attempted)
    {
        result->uncertain = 1;
        fid->poisoned = 1;
    }
    pthread_mutex_unlock(&client->lock);
}


/* EN/RU: See client.h; no metadata preflight or read replay.
 * См. client.h; без metadata preflight и read replay. */
void styxclientread(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxTransfer *result)
{
    chunk(fid, offset, bytes, size, 0, result);
}


/* EN/RU: See client.h; Rerror cannot prove that a write had zero effects.
 * См. client.h; Rerror не доказывает отсутствие write effects. */
void styxclientwrite(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxTransfer *result)
{
    chunk(fid, offset, (void *)bytes, size, 1, result);
}
