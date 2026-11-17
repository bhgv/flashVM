/* Copyright (c) 2026 bhgv
 * EN: Managed resource reads over real Twalk/Topen/Twrite/Tread/Tclunk and S-record framing.
 * RU: Managed resource reads через настоящие Twalk/Topen/Twrite/Tread/Tclunk и S-record framing.
 */
#include "client.h"
#include "protocol.h"
#include <string.h>


/* EN/RU: Send one complete record with confirmed partial acknowledgements; no replay.
 * Shared stream offset includes both directions, matching existing StyxOpen semantics.
 * Передать record с confirmed partial acknowledgements; без replay, общий stream offset. */
static int sendrecord(StyxFid *fid, unsigned long long *offset, const void *bytes, size_t size)
{
    size_t done = 0;
    while (done < size)
    {
        StyxTransfer result;
        styxclientwrite(fid, *offset, (const unsigned char *)bytes+done, size-done, &result);
        if (!result.success || !result.confirmed || result.uncertain ||
            !result.count || result.count > size-done)
        {
            return 0;
        }
        done += result.count;
        *offset += result.count;
    }
    return 1;
}


/* EN/RU: Read one bounded complete record, retaining fragmented framing;1/0.
 * Bytes/view remain borrowed from output until next call; EOF before END is failure.
 * Прочитать один bounded record с fragment retention;1/0, EOF до END — ошибка. */
static int receive(StyxFid *fid, unsigned long long *offset, unsigned char *bytes,
    size_t *size, size_t *previous, FVM_SRecord *view)
{
    size_t consumed;
    if (*previous)
    {
        *size -= *previous;
        memmove(bytes, bytes+*previous, *size);
        *previous = 0;
    }
    for (;;)
    {
        FVM_SResult status = fvm_srecord_read(bytes, *size, MOVIE_FRAME, view, &consumed);
        if (status == FVM_S_OK)
        {
            *previous = consumed;
            return 1;
        }
        if (status != FVM_S_MORE || *size == MOVIE_FRAME)
        {
            return 0;
        }
        StyxTransfer result;
        /* Keep any following record bytes; transport read boundaries are not message boundaries. */
        styxclientread(fid, *offset, bytes+*size, MOVIE_FRAME-*size, &result);
        if (!result.success || !result.confirmed || result.uncertain ||
            !result.count || result.count > MOVIE_FRAME-*size)
        {
            return 0;
        }
        *size += result.count;
        *offset += result.count;
    }
}


/* EN/RU: See client.h; all fid/stream buffers retire on success, sink refusal or protocol failure.
 * См. client.h; fid/buffers освобождаются при успехе, sink refusal и protocol failure. */
FVM_Result styxmoviefetch(unsigned int purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *write_userdata, char *resolved, size_t capacity, void *userdata)
{
    unsigned char buffer[MOVIE_FRAME], request[MOVIE_FRAME];
    size_t size, previous = 0, rn, bn;
    unsigned long long offset = 0;
    FVM_SRecord record;
    FVM_SBindings bindings;
    FVM_SBinding slot;
    FVM_Result result = FVM_ERROR;
    int begun = 0;
    if (!userdata || !reference || !write || !resolved || !capacity ||
        (purpose != FVM_DESKTOP_FETCH_CODE && purpose != FVM_DESKTOP_FETCH_DATA))
    {
        return FVM_INVALID_ARGUMENT;
    }
    rn = strlen(reference);
    bn = base ? strlen(base) : 0;
    if (!rn || rn >= MOVIE_PATH || bn >= MOVIE_PATH)
    {
        return FVM_INVALID_ARGUMENT;
    }
    StyxFid *fid = styxclientwalk(userdata, ".");
    if (!fid)
    {
        return FVM_ERROR;
    }
    if (!styxclientopen(fid, 2) || !movie_binding(buffer, &size) ||
        !sendrecord(fid, &offset, buffer, size))
    {
        goto done;
    }
    request[0] = purpose == FVM_DESKTOP_FETCH_CODE ? MOVIE_CODE : MOVIE_DATA;
    request[1] = (unsigned char)rn;
    request[2] = (unsigned char)(rn>>8);
    request[3] = (unsigned char)bn;
    request[4] = (unsigned char)(bn>>8);
    memcpy(request+5, reference, rn);
    if (bn)
    {
        memcpy(request+5+rn, base, bn);
    }
    if (!movie_record(buffer, &size, MOVIE_REQUEST, request, 5+rn+bn) ||
        !sendrecord(fid, &offset, buffer, size))
    {
        goto done;
    }
    fvm_sbindings_init(&bindings, &slot, 1);
    size = 0;
    while (receive(fid, &offset, buffer, &size, &previous, &record))
    {
        if (record.tag == FVM_SRECORD_CONTROL_TAG)
        {
            size_t consumed;
            if (begun || bindings.count ||
                fvm_sbindings_apply(&bindings, buffer, size, &consumed) != FVM_S_OK || !movie_bound(&bindings))
            {
                break;
            }
            continue;
        }
        if (record.tag != MOVIE_TAG || !movie_bound(&bindings) || !record.size)
        {
            break;
        }
        unsigned kind = record.payload[0];
        if (kind == MOVIE_BEGIN && !begun && record.size > 1 && record.size-1 < capacity &&
            record.payload[1] == '/' && !memchr(record.payload+1, 0, record.size-1))
        {
            memcpy(resolved, record.payload+1, record.size-1);
            resolved[record.size-1] = 0;
            begun = 1;
        }
        else if (kind == MOVIE_BYTES && begun && record.size > 1)
        {
            result = write(record.payload+1, record.size-1, write_userdata);
            if (result != FVM_OK)
            {
                goto done;
            }
            result = FVM_ERROR;
        }
        else if (kind == MOVIE_END && record.size == 2)
        {
            result = begun && record.payload[1] == 0 ? FVM_OK : FVM_ERROR;
            break;
        }
        else
        {
            break;
        }
    }
done:
    styxclientclunk(fid);
    return result;
}
