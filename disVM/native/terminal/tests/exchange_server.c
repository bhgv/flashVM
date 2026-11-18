/* Copyright (c) 2026 bhgv
 * EN: Original libstyx peer dispatches one S-record request to the actual VFS resource service.
 * RU: Исходный libstyx peer передаёт один S-запрос настоящему VFS resource service.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#include "request_payload.h"
#ifndef STYX_EXCHANGE_CASE
#define STYX_EXCHANGE_CASE 0 /* Normal executable resource intent. */
#endif
static unsigned char offered[2048]; /* Original server-owner outgoing stream. */
static size_t offered_size; /* Published outgoing bytes. */
static unsigned char incoming[MOVIE_FRAME]; /* Bounded original server-owner request decoder. */
static FVM_SStream decoder; /* Server-owner fragmented S-record decoder. */
static StyxRead *pending; /* Original deferred idle read, one owner. */
static size_t position; /* Confirmed request-stream write prefix. */
static unsigned messages; /* Complete received fixture records. */
static unsigned char request[MOVIE_FRAME]; /* Copied request retained only for fixture-gated late response. */
static size_t request_size; /* Exact copied request bytes. */


/* EN/RU: Call actual service across the existing native SDK boundary, no original Inferno type leakage.
 * Вызвать настоящий service через native SDK boundary, без утечки типов Inferno.
 */
extern int exchangereply(const void *, size_t, void (*)(void *, const void *, size_t), void *);


/* EN/RU: Append exactly one framed record; bytes borrowed through copy, overflow fails fixture.
 * Добавить одну S-запись, bytes заимствованы до копирования; overflow завершает fixture.
 */
static void append(unsigned long tag, const void *bytes, size_t size)
{
    size_t encoded; /* Full checked frame length. */
    if (fvm_srecord_write(tag, bytes, size, offered + offered_size,
        sizeof(offered) - offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
}


/* EN/RU: Reframe one checked real service body onto the independently selected fixture RX tag.
 * Перенести checked body настоящего service на отдельно выбранный RX tag fixture.
 */
static void emit(void *unused, const void *bytes, size_t size)
{
    USED(unused);
    append(23, bytes, size);
}


/* EN/RU: Initial bindings, opaque data and UI are independent of any resource request.
 * Начальные bindings, opaque data и UI независимы от resource request.
 */
static void initialize(void)
{
    if (offered_size)
    {
        return;
    }
    FVM_SBinding bindings[] = { /* Agreed fixture schemas, no production numeric tag assignment. */
        {23, MOVIE_VERSION, sizeof(MOVIE_TYPE) - 1, MOVIE_TYPE},
        {5, 1, 11, "test.opaque"}, {17, 1, 13, "test.ide.fvd1"}
    };
    unsigned char bytes[256]; /* Temporary declaration body. */
    size_t size; /* Encoded body extent. */
    if (fvm_sbindings_payload_write(bindings, 3, bytes, sizeof(bytes), &size) != FVM_S_OK ||
        fvm_sstream_init(&decoder, incoming, sizeof(incoming), MOVIE_FRAME) != FVM_S_OK)
    {
        exit(3);
    }
    append(0, bytes, size);
    append(5, "other", 5);
    static const char ui[] = "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
    append(17, ui, sizeof(ui) - 1);
    if (STYX_EXCHANGE_CASE == 10)
    {
        static const unsigned char stale[] = {2, '/', 'o', 'l', 'd'}; /* Deliberately prequeued reply before any request. */
        append(23, stale, sizeof(stale));
    }
}


/* EN/RU: Original idle-read retirement, called once on its native server owner.
 * Одноразовое завершение исходного idle-read на native server owner.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Original retained caller handle. */
    pending = nil;
    styxreadfree(read);
}


/* EN/RU: Complete one actual deferred read after appending responses, preserving three-byte fragmentation.
 * Завершить настоящий deferred read после ответа, сохраняя дробление по три байта.
 */
static void publish(void)
{
    if (pending)
    {
        vlong offset = styxreadoffset(pending); /* Borrowed until reply invokes release. */
        ulong count = styxreadlimit(pending); /* Exact peer buffer limit. */
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


/* EN/RU: Return original S-stream prefixes or defer real idle input; never manufacture EOF.
 * Вернуть prefixes S-stream либо отложить настоящий idle input, без ложного EOF.
 */
char *terminalread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    initialize();
    if (offset < 0 || (uvlong)offset > offered_size)
    {
        return Ebadarg;
    }
    size_t size = offered_size - (size_t)offset; /* Available exact prefix. */
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


/* EN/RU: Validate one received request; only fixture control may release an intentionally delayed reply.
 * Проверить запрос; только fixture control разрешает намеренно задержанный ответ.
 */
static int accept(const FVM_SRecord *record)
{
    if (!messages)
    {
        char reference[MOVIE_PATH]; /* Exact decoded reference. */
        char base[MOVIE_PATH]; /* Exact decoded base. */
        unsigned purpose; /* Explicit wire intent. */
        if (record->tag != 101 || !movie_request_parse(record->payload, record->size, &purpose, reference, base) ||
            strcmp(reference, STYX_EXCHANGE_CASE == 2 ? "/denied.swf" : "/ok.swf") ||
            strcmp(base, "/base.swf") || purpose != (STYX_EXCHANGE_CASE == 1 ? MOVIE_DATA : MOVIE_CODE))
        {
            return 0;
        }
        memcpy(request, record->payload, record->size);
        request_size = record->size;
        append(9, "request", 7);
        if (STYX_EXCHANGE_CASE != 4 && STYX_EXCHANGE_CASE != 5 &&
            STYX_EXCHANGE_CASE != 7 && STYX_EXCHANGE_CASE != 8 &&
            !exchangereply(request, request_size, emit, nil))
        {
            return 0;
        }
    }
    else if (messages == 1 && record->tag == 102 && record->size == 4 && !memcmp(record->payload, "next", 4))
    {
        if (!exchangereply(request, request_size, emit, nil))
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    messages++;
    publish();
    return 1;
}


/* EN/RU: Acknowledge only exact two-byte prefixes, dispatch each checked record once without replay.
 * Подтверждать точные prefixes по два байта, один dispatch каждой проверенной записи без replay.
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
    size_t at = 0; /* Consumed prefix of the original Twrite. */
    while (at < *count)
    {
        FVM_SRecord record; /* Checked complete record borrowing decoder storage. */
        size_t used = 0; /* Exact consumed prefix. */
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
