/* Copyright (c) 2026 bhgv
 * EN: Original libstyx wire peer sends resource requests and independently validates native server replies.
 * RU: Исходный libstyx wire peer посылает запросы и независимо проверяет ответы native server.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#include "reply.h"
#ifndef STYX_SERVER_CASE
#define STYX_SERVER_CASE 0 /* Valid one-request server exchange. */
#endif
static unsigned char offered[2048]; /* Original server-owner output stream. */
static size_t offered_size; /* Encoded offered bytes. */
static unsigned char incoming[MOVIE_FRAME]; /* Original server-owner reply decoder storage. */
static FVM_SStream decoder; /* Incremental exact S-record decoder. */
static StyxRead *pending; /* Original native deferred idle read. */
static size_t position; /* Confirmed incoming write prefix. */
static MovieReply reply; /* Native validation state, no FlashVM execution. */
static char origin[MOVIE_PATH]; /* Published only after real server success. */
static unsigned char collected[16]; /* Independent native binary collector. */
static size_t collected_size; /* Exact staged byte count. */
static unsigned requests; /* Fixture-only request publication guard. */


/* EN/RU: Append one fully framed record, borrowing bytes through encoding; overflow fails fixture.
 * Добавить одну полную S-запись, bytes заимствованы; overflow завершает fixture.
 */
static void append(unsigned long tag, const void *bytes, size_t size)
{
    size_t encoded; /* Full record extent. */
    if (fvm_srecord_write(tag, bytes, size, offered + offered_size,
        sizeof(offered) - offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
}


/* EN/RU: Offer one independent existing request vector, optionally malformed/duplicated for refusal.
 * Выдать независимый вектор существующего запроса, malformed/повтор по варианту fixture.
 */
static void offerrequest(void)
{
    static const unsigned char normal[] = {1, 1, 7, 0, 0, 0, '/', 'o', 'k', '.', 's', 'w', 'f'};
    static const unsigned char denied[] = {1, 1, 11, 0, 0, 0, '/', 'd', 'e', 'n', 'i', 'e', 'd', '.', 's', 'w', 'f'};
    static const unsigned char invalid[] = {99}; /* Not a valid existing request opcode. */
    if (requests++)
    {
        exit(3);
    }
    if (STYX_SERVER_CASE == 2)
    {
        append(23, denied, sizeof(denied));
    }
    else if (STYX_SERVER_CASE == 3)
    {
        append(23, invalid, sizeof(invalid));
    }
    else
    {
        append(23, normal, sizeof(normal));
    }
    if (STYX_SERVER_CASE == 8)
    {
        append(23, denied, sizeof(denied));
    }
}


/* EN/RU: Collect exact checked native reply bytes on original server owner; no VM/cross-owner calls.
 * Сохранить checked reply bytes на исходном server owner, без VM/cross-owner calls.
 */
static FVM_Result staging(const void *bytes, size_t size, void *unused)
{
    USED(unused);
    if (size > sizeof(collected) - collected_size)
    {
        return FVM_ERROR;
    }
    memmove(collected + collected_size, bytes, size);
    collected_size += size;
    return FVM_OK;
}


/* EN/RU: Build existing negotiated profile and independent UI/opaque traffic before resource input.
 * Собрать существующий negotiated profile и независимый UI/opaque traffic до resource input.
 */
static void initialize(void)
{
    if (offered_size)
    {
        return;
    }
    FVM_SBinding bindings[] = { /* Fixture numeric associations, not production assignments. */
        {23, MOVIE_VERSION, sizeof(MOVIE_TYPE) - 1, MOVIE_TYPE},
        {5, 1, 11, "test.opaque"}, {17, 1, 13, "test.ide.fvd1"}
    };
    unsigned char bytes[256]; /* Temporary declaration body. */
    size_t size; /* Exact body length. */
    if (fvm_sbindings_payload_write(bindings, 3, bytes, sizeof(bytes), &size) != FVM_S_OK ||
        fvm_sstream_init(&decoder, incoming, sizeof(incoming), MOVIE_FRAME) != FVM_S_OK ||
        !movie_reply_init(&reply, staging, nil, origin, sizeof(origin)))
    {
        exit(3);
    }
    append(0, bytes, size);
    append(5, "other", 5);
    static const char ui[] = "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
    append(17, ui, sizeof(ui) - 1);
    if (STYX_SERVER_CASE != 4 && STYX_SERVER_CASE != 5 && STYX_SERVER_CASE != 10)
    {
        offerrequest();
    }
}


/* EN/RU: Original deferred read release, one owner and one consuming free.
 * Освобождение исходного deferred read одним owner, consuming free один раз.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Original retained handle. */
    pending = nil;
    styxreadfree(read);
}


/* EN/RU: Wake original idle read after appending a receipt/request, preserving three-byte prefixes.
 * Завершить исходный idle read после добавления receipt/request, сохраняя prefixes по три байта.
 */
static void publish(void)
{
    if (pending)
    {
        vlong offset = styxreadoffset(pending); /* Borrowed until reply invokes release. */
        ulong count = styxreadlimit(pending); /* Exact peer capacity. */
        if (offset < 0 || (uvlong)offset >= offered_size)
        {
            exit(3);
        }
        if (count > offered_size - (size_t)offset)
        {
            count = offered_size - (size_t)offset;
        }
        if (count > 3)
        {
            count = 3;
        }
        if (!styxreadreply(pending, offered + offset, count, nil))
        {
            exit(3);
        }
    }
}


/* EN/RU: Return real fragmented S-record input or defer idle Tread; never false EOF.
 * Вернуть fragmented S-input либо отложить idle Tread, без ложного EOF.
 */
char *terminalread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    initialize();
    if (offset < 0 || (uvlong)offset > offered_size)
    {
        return Ebadarg;
    }
    size_t size = offered_size - (size_t)offset; /* Available prefix. */
    if (!size)
    {
        if (pending)
        {
            return Eperm;
        }
        pending = styxdeferread(server, nil, release);
        *count = 0;
        return pending ? nil : Enomem;
    }
    if (size > *count)
    {
        size = *count;
    }
    if (size > 3)
    {
        size = 3;
    }
    memmove(bytes, offered + offset, size);
    *count = size;
    return nil;
}


/* EN/RU: Validate actual server resource replies, with independent binary oracle; other records stay separate.
 * Проверить реальные ответы resource server независимым binary oracle, отдельно от других записей.
 */
static int accept(const FVM_SRecord *record)
{
    if (record->tag == 102 && record->size == 5 && !memcmp(record->payload, "start", 5) && STYX_SERVER_CASE == 10)
    {
        offerrequest();
        publish();
        return 1;
    }
    if (record->tag == 102 && record->size == 5 && !memcmp(record->payload, "alive", 5))
    {
        append(9, "alive", 5);
        publish();
        return 1;
    }
    if (record->tag != 101)
    {
        return 0;
    }
    int status = movie_reply_accept(&reply, record->payload, record->size); /* Existing reply parser, original server owner. */
    if (status == MOVIE_REPLY_DONE)
    {
        static const unsigned char expected[] = {0, 128, 255, 0, 'x'}; /* Independent native content oracle. */
        if (STYX_SERVER_CASE == 2 || STYX_SERVER_CASE == 10 || collected_size != sizeof(expected) ||
            memcmp(collected, expected, sizeof(expected)) || strcmp(origin, "/ok.swf"))
        {
            return 0;
        }
        append(9, "done", 4);
        publish();
    }
    else if (status == MOVIE_REPLY_FAILED)
    {
        if ((STYX_SERVER_CASE != 2 && STYX_SERVER_CASE != 10) || record->size != 2 ||
            record->payload[0] != MOVIE_END || record->payload[1] != 1 || collected_size)
        {
            return 0;
        }
        append(9, "done", 4);
        publish();
    }
    return 1;
}


/* EN/RU: Preserve precise two-byte write acknowledgements across independently validated S-records.
 * Сохранить точные подтверждения записи по два байта через checked S-records.
 */
char *terminalwrite(char *bytes, ulong *count, vlong offset)
{
    initialize();
    if (offset < 0 || (uvlong)offset != position)
    {
        return Ebadarg;
    }
    if (*count > 2)
    {
        *count = 2;
    }
    size_t at = 0; /* Current Twrite consumed prefix. */
    while (at < *count)
    {
        FVM_SRecord record; /* Checked decoder view, borrowed until reset. */
        size_t used = 0; /* Exact consumed input. */
        FVM_SResult status = fvm_sstream_push(&decoder, bytes + at, *count - at, &record, &used);
        at += used;
        if (status == FVM_S_OK)
        {
            if (!accept(&record))
            {
                return Ebadarg;
            }
            fvm_sstream_reset(&decoder);
        }
        else if (status != FVM_S_MORE || !used)
        {
            return Ebadarg;
        }
    }
    position += *count;
    return nil;
}
