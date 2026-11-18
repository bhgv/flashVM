/* Copyright (c) 2026 bhgv
 * EN: Original libstyx S-record offer stream with independent UI/opaque records and gated generations.
 * RU: Исходный libstyx S-offer stream с независимыми UI/opaque records и gated generations.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#include "announcement.h"
#ifndef STYX_OFFER_CASE
#define STYX_OFFER_CASE 0 /* Initial ready generation. */
#endif
static unsigned char offered[2048]; /* Original server-owner offered S-stream. */
static size_t offered_size; /* Encoded offered bytes, including appended fixture updates. */
static unsigned char input[256]; /* Server-owner command decoder storage. */
static FVM_SStream decoder; /* Server-owner exact fragmented S-record decoder. */
static StyxRead *pending; /* Original deferred idle Tread, released once on owner. */
static size_t position; /* Confirmed incoming write extent. */


/* EN/RU: Append one framed record, input borrowed only through encoding; overflow fails fixture.
 * Добавить одну S-запись, input заимствован до кодирования; overflow завершает fixture.
 */
static void append(unsigned long tag, const void *bytes, size_t size)
{
    size_t encoded; /* Full frame extent. */
    if (fvm_srecord_write(tag, bytes, size, offered + offered_size,
        sizeof(offered) - offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
}


/* EN/RU: Append the existing production offer format, no address/path capability on wire.
 * Добавить существующий production offer format без address/path capability на wire.
 */
static void offer(unsigned state, unsigned long long generation)
{
    size_t encoded; /* Exact checked frame bytes. */
    if (!movie_offer_record(23, state, generation, offered + offered_size,
        sizeof(offered) - offered_size, &encoded))
    {
        exit(3);
    }
    offered_size += encoded;
}


/* EN/RU: Build the existing binding profile and one autonomous initial notice unless deliberately gated.
 * Собрать existing binding profile и initial notice, если он намеренно не задержан.
 */
static void initialize(void)
{
    if (offered_size)
    {
        return;
    }
    FVM_SBinding bindings[] = { /* Fixture tags, never production assignments. */
        {23, STYX_OFFER_CASE == 6 ? 2 : MOVIE_OFFER_VERSION, sizeof(MOVIE_OFFER_TYPE) - 1, MOVIE_OFFER_TYPE},
        {5, 1, 11, "test.opaque"}, {17, 1, 13, "test.ide.fvd1"}
    };
    unsigned char bytes[256]; /* Temporary tag-zero body. */
    size_t size; /* Encoded declaration size. */
    if (fvm_sbindings_payload_write(bindings, 3, bytes, sizeof(bytes), &size) != FVM_S_OK ||
        fvm_sstream_init(&decoder, input, sizeof(input), sizeof(input)) != FVM_S_OK)
    {
        exit(3);
    }
    append(0, bytes, size);
    append(5, "other", 5);
    static const char ui[] = "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
    append(17, ui, sizeof(ui) - 1);
    if (STYX_OFFER_CASE == 7)
    {
        static const unsigned char invalid[] = {MOVIE_OFFER_READY}; /* Missing generation. */
        append(23, invalid, sizeof(invalid));
    }
    else if (STYX_OFFER_CASE != 2 && STYX_OFFER_CASE != 11)
    {
        offer(MOVIE_OFFER_READY, 1);
    }
}


/* EN/RU: Release original deferred handle once on server owner, no native/guest wait.
 * Освободить original deferred handle один раз на server owner, без native/guest wait.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Original retained handle. */
    pending = nil;
    styxreadfree(read);
}


/* EN/RU: Complete one original pending Tread after appending updates, preserving three-byte prefixes.
 * Завершить original pending Tread после update, сохраняя prefixes по три байта.
 */
static void publish(void)
{
    if (pending)
    {
        vlong offset = styxreadoffset(pending); /* Borrowed until reply invokes release. */
        ulong count = styxreadlimit(pending); /* Original buffer capacity. */
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


/* EN/RU: Read real fragmented records or defer idle input, no fabricated EOF.
 * Прочитать fragmented records либо отложить idle input, без ложного EOF.
 */
char *terminalread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    initialize();
    if (offset < 0 || (uvlong)offset > offered_size)
    {
        return Ebadarg;
    }
    size_t size = offered_size - (size_t)offset; /* Available precise prefix. */
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


/* EN/RU: Test-only commands gate exact offer changes; these are not new production control schemas.
 * Только test commands управляют точными offer changes; это не новые production control schemas.
 */
static int command(const FVM_SRecord *record)
{
    if (record->tag != 102)
    {
        return 0;
    }
    if (record->size == 5 && !memcmp(record->payload, "ready", 5))
    {
        offer(MOVIE_OFFER_READY, 1);
    }
    else if (record->size == 5 && !memcmp(record->payload, "newer", 5))
    {
        offer(MOVIE_OFFER_READY, 2);
    }
    else if (record->size == 6 && !memcmp(record->payload, "closed", 6))
    {
        offer(MOVIE_OFFER_CLOSED, 2);
    }
    else if (record->size == 8 && !memcmp(record->payload, "conflict", 8))
    {
        offer(MOVIE_OFFER_CLOSED, 1);
    }
    else if (record->size == 3 && !memcmp(record->payload, "old", 3))
    {
        offer(MOVIE_OFFER_CLOSED, 1);
        offer(MOVIE_OFFER_READY, 3);
    }
    else if (record->size == 5 && !memcmp(record->payload, "alive", 5))
    {
        append(9, "alive", 5);
    }
    else
    {
        return 0;
    }
    publish();
    return 1;
}


/* EN/RU: Exact two-byte write acknowledgement with complete S-record command parsing, no raw insertion.
 * Точный ACK по два байта с полным S-парсингом commands, без raw insertion.
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
    size_t at = 0; /* Confirmed prefix of this Twrite. */
    while (at < *count)
    {
        FVM_SRecord record; /* Complete checked body, borrowed until reset. */
        size_t used = 0; /* Exact consumed extent. */
        FVM_SResult result = fvm_sstream_push(&decoder, bytes + at, *count - at, &record, &used);
        at += used;
        if (result == FVM_S_OK)
        {
            if (!command(&record))
            {
                return Ebadarg;
            }
            fvm_sstream_reset(&decoder);
        }
        else if (result != FVM_S_MORE || !used)
        {
            return Ebadarg;
        }
    }
    position += *count;
    return nil;
}
