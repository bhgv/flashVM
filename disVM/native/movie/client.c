/* Copyright (c) 2026 bhgv
 * EN: Managed resource reads over real Twalk/Topen/Twrite/Tread/Tclunk and S-record framing.
 * RU: Managed resource reads через настоящие Twalk/Topen/Twrite/Tread/Tclunk и S-record framing.
 */
#include "fetch_internal.h"
#include "mount_cancel.h"
#include "protocol.h"
#include "reply.h"
#include "request_payload.h"
#include <string.h>


/* EN/RU: Send borrowed bytes through exact client/fid, advancing offset only for confirmed prefixes; 1/0.
 * Shared stream offset includes both directions. Each RPC gets a fresh scoped token; no replay.
 * Передать borrowed bytes через exact client/fid, offset только по confirmed prefixes; 1/0, без replay. */
static int sendrecord(StyxClient *client, StyxFid *fid, unsigned long long *offset, const void *bytes, size_t size)
{
    size_t done = 0;
    while (done < size)
    {
        StyxTransfer result;
        movie_transfer(client, fid, *offset, (unsigned char *)bytes+done, size-done, 1, &result);
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


/* EN/RU: Read one bounded record on exact client/fid, retaining framing in borrowed bytes/size/previous; 1/0.
 * view borrows bytes until the next call; offset advances only for confirmed data. EOF before END fails.
 * Прочитать bounded record через exact client/fid с borrowed framing state; 1/0, EOF до END — ошибка. */
static int receive(StyxClient *client, StyxFid *fid, unsigned long long *offset, unsigned char *bytes,
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
        movie_transfer(client, fid, *offset, bytes+*size, MOVIE_FRAME-*size, 0, &result);
        if (!result.success || !result.confirmed || result.uncertain ||
            !result.count || result.count > MOVIE_FRAME-*size)
        {
            return 0;
        }
        *size += result.count;
        *offset += result.count;
    }
}


/* EN/RU: Native sink remains on its caller, with data and cancellation checked around one invocation.
 * Native sink остаётся на caller, data/cancellation проверяются вокруг одного invocation.
 */
typedef struct MovieSink
{
    FVM_DesktopWrite write; /* Borrowed native sink through this fetch only. */
    void *state;            /* Borrowed caller-owned sink context. */
} MovieSink;


/* EN/RU: Never start a sink after observed cancellation; retain a sink's own refusal result.
 * Не начинать sink после observed cancellation; сохранить собственный отказ sink.
 */
static FVM_Result staging(const void *bytes, size_t size, void *state)
{
    MovieSink *sink = state; /* Owner-local context, no cross-thread callback. */
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    FVM_Result result = sink->write(bytes, size, sink->state);
    return result == FVM_OK && dvm_cancel_requested(dvm_cancel_current()) ? FVM_ERROR : result;
}


/* EN/RU: See fetch_internal.h; all fid/stream buffers retire on success, sink refusal or protocol failure.
 * См. fetch_internal.h; fid/buffers освобождаются при успехе, sink refusal и protocol failure. */
FVM_Result movie_fetch_fid(StyxClient *client, StyxFid *fid, unsigned int purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *write_userdata, char *resolved, size_t capacity)
{
    unsigned char buffer[MOVIE_FRAME], request[MOVIE_FRAME];
    char origin[MOVIE_PATH]; /* Private final origin until cleanup and scope checks succeed. */
    MovieSink sink = {write, write_userdata}; /* Native owner-local callback gate. */
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
        styxmountclunk(client, fid);
        return FVM_INVALID_ARGUMENT;
    }
    size_t request_size; /* Complete checked payload including existing REQUEST opcode. */
    if (!movie_request_payload(request, sizeof(request), &request_size,
        purpose == FVM_DESKTOP_FETCH_CODE ? MOVIE_CODE : MOVIE_DATA, reference, base))
    {
        styxmountclunk(client, fid);
        return FVM_INVALID_ARGUMENT;
    }
    if (!movie_reply_init(&reply, staging, &sink, origin, capacity < sizeof(origin) ? capacity : sizeof(origin)) ||
        !styxmountopen(client, fid, 2) || !movie_binding(buffer, &size) ||
        !sendrecord(client, fid, &offset, buffer, size))
    {
        goto done;
    }
    if (!movie_record(buffer, &size, MOVIE_REQUEST, request + 1, request_size - 1) ||
        !sendrecord(client, fid, &offset, buffer, size))
    {
        goto done;
    }
    fvm_sbindings_init(&bindings, &slot, 1);
    size = 0;
    while (receive(client, fid, &offset, buffer, &size, &previous, &record))
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
    styxmountclunk(client, fid);
    if (result == FVM_OK)
    {
        if (dvm_cancel_requested(dvm_cancel_current()))
        {
            result = FVM_ERROR;
        }
        else
        {
            memcpy(resolved, origin, reply.path_size + 1);
        }
    }
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
    return movie_fetch_fid(userdata, styxmountwalkcancel(userdata, NULL, "."), purpose, reference, base,
        write, write_userdata, resolved, capacity);
}
