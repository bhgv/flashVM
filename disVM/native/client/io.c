/* Copyright (c) 2026 bhgv
 * EN: Precise remote chunk acknowledgements, mode ceilings and uncertain-effect poison.
 * RU: Точные remote chunk acknowledgements, mode ceilings и uncertain-effect poison.
 */
#include "internal.h"


/* EN/RU: One native data operation under connection lock; retain exact Rread/Rwrite count.
 * Одна native data-операция под lock; сохранить точный Rread/Rwrite count. */
static void chunk(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, int writing, StyxCancel *cancel, int watching, StyxTransfer *result)
{
    if (!result)
    {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->confirmed = 1;
    if (!fid || !fid->opened || offset > 0x7fffffffffffffffULL || (!bytes && size) ||
        (writing ? fid->mode == 0 : fid->mode == 1))
    {
        return;
    }
    /* EN: A directory Tread at zero starts a new enumeration on the same fid.
     * RU: Tread каталога с нуля начинает новый обход того же fid. */
    if (fid->poisoned && (writing || offset || fid->mode != 0 || !(fid->qid.type & QTDIR)))
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
    int status = watching ? styxrpcwatch(client, &request, &reply, cancel) : styxrpccancel(client, &request, &reply, cancel);
    result->cancelled = client->cancelled;
    if (status == 1 && reply.count > request.count)
    {
        styxrpcbad(client);
        status = -1;
    }
    if (status == 1)
    {
        fid->poisoned = 0;
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
    chunk(fid, offset, bytes, size, 0, NULL, 0, result);
}


/* EN/RU: See client.h; Rerror cannot prove that a write had zero effects.
 * См. client.h; Rerror не доказывает отсутствие write effects. */
void styxclientwrite(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxTransfer *result)
{
    chunk(fid, offset, (void *)bytes, size, 1, NULL, 0, result);
}


/* EN/RU: See client.h; explicit native cancellation is accepted only by fd owner mode.
 * См. client.h; explicit cancellation принимается только fd owner mode. */
void styxclientreadcancel(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result)
{
    if (fid && fid->client->mux)
    {
        chunk(fid, offset, bytes, size, 0, cancel, 0, result);
    }
    else if (result)
    {
        memset(result, 0, sizeof(*result));
        result->confirmed = 1;
    }
}


/* EN/RU: See client.h; preserve original write acknowledgement if it wins the flush race.
 * См. client.h; сохранить original write acknowledgement при победе над flush. */
void styxclientwritecancel(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result)
{
    if (fid && fid->client->mux)
    {
        chunk(fid, offset, (void *)bytes, size, 1, cancel, 0, result);
    }
    else if (result)
    {
        memset(result, 0, sizeof(*result));
        result->confirmed = 1;
    }
}


/* EN/RU: See client.h; explicit cancel is mandatory, fd-owner keeps remaining RPC capacity.
 * См. client.h; explicit cancel обязателен, fd-owner сохраняет оставшуюся RPC capacity. */
void styxclientreadwatch(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result)
{
    if (fid && cancel && styxclientwatchable(fid->client))
    {
        chunk(fid, offset, bytes, size, 0, cancel, 1, result);
    }
    else if (result)
    {
        memset(result, 0, sizeof(*result));
        result->confirmed = 1;
    }
}
