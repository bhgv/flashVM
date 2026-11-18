/* Copyright (c) 2026 bhgv
 * EN: Managed resource reads over real Twalk/Topen/Twrite/Tread/Tclunk and S-record framing.
 * RU: Managed resource reads через настоящие Twalk/Topen/Twrite/Tread/Tclunk и S-record framing.
 */
#include "client.h"
#include "protocol.h"
#include "reply.h"
#include "request_payload.h"
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
FVM_Result movie_fetch_fid(StyxFid *fid, unsigned int purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *write_userdata, char *resolved, size_t capacity)
{
    unsigned char buffer[MOVIE_FRAME], request[MOVIE_FRAME];
    size_t size, previous = 0;
    unsigned long long offset = 0;
    FVM_SRecord record;
    FVM_SBindings bindings;
    FVM_SBinding slot;
    FVM_Result result = FVM_ERROR;
    MovieReply reply; /* Owner-local validator shared with typed Unibus consumers. */
    if (!fid)
    {
        return FVM_ERROR;
    }
    if (!reference || !write || !resolved || !capacity ||
        (purpose != FVM_DESKTOP_FETCH_CODE && purpose != FVM_DESKTOP_FETCH_DATA))
    {
        styxclientclunk(fid);
        return FVM_INVALID_ARGUMENT;
    }
    size_t request_size; /* Complete checked payload including existing REQUEST opcode. */
    if (!movie_request_payload(request, sizeof(request), &request_size,
        purpose == FVM_DESKTOP_FETCH_CODE ? MOVIE_CODE : MOVIE_DATA, reference, base))
    {
        styxclientclunk(fid);
        return FVM_INVALID_ARGUMENT;
    }
    if (!movie_reply_init(&reply, write, write_userdata, resolved, capacity) ||
        !styxclientopen(fid, 2) || !movie_binding(buffer, &size) ||
        !sendrecord(fid, &offset, buffer, size))
    {
        goto done;
    }
    if (!movie_record(buffer, &size, MOVIE_REQUEST, request + 1, request_size - 1) ||
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
            if (reply.stage != 0 || bindings.count ||
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
        int accepted = movie_reply_accept(&reply, record.payload, record.size); /* One checked service payload. */
        if (accepted != MOVIE_REPLY_MORE)
        {
            result = movie_reply_result(&reply);
            break;
        }
    }
done:
    styxclientclunk(fid);
    return result;
}


/* EN/RU: See client.h; preserve ordinary attached-root lookup and pre-I/O argument refusal.
 * См. client.h; сохранить attached-root lookup и отказ invalid arguments до I/O. */
FVM_Result styxmoviefetch(unsigned int purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *write_userdata, char *resolved, size_t capacity, void *userdata)
{
    if (!userdata || !reference || !*reference || strlen(reference) >= MOVIE_PATH ||
        (base && strlen(base) >= MOVIE_PATH) || !write || !resolved || !capacity ||
        (purpose != FVM_DESKTOP_FETCH_CODE && purpose != FVM_DESKTOP_FETCH_DATA))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return movie_fetch_fid(styxclientwalk(userdata, "."), purpose, reference, base,
        write, write_userdata, resolved, capacity);
}
