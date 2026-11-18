/* Copyright (c) 2026 bhgv
 * EN: Original libstyx fixture fills one route queue after setup, keeping idle reads cancellable.
 * RU: Исходный libstyx fixture заполняет выбранную route queue после setup; idle read отменяем.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#include "flashvm/sbindings.h"
#ifndef STYX_RETIREMENT_CASE
#define STYX_RETIREMENT_CASE 0 /* General queue, real uncertain TX failure. */
#endif
static unsigned char offered[65536]; /* Original server-owner offered S-stream. */
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


/* EN/RU: Initialize opaque/UI input and the fixture-only outgoing binding registry.
 * Инициализировать opaque/UI input и test-only registry исходящих bindings.
 */
static void initialize(void)
{
    if (offered_size)
    {
        return;
    }
    FVM_SBinding initial[] = { /* Independent fixture traffic, no offer before publication. */
        {5, 1, 11, "test.opaque"}, {17, 1, 13, "test.ide.fvd1"},
        {23, 1, 12, "test.blocked"}, {24, 1, 19, "flashvm.movie.offer"}
    };
    unsigned char bytes[256]; /* Temporary declaration body. */
    size_t size; /* Exact encoded body extent. */
    if (fvm_sbindings_payload_write(initial, 4, bytes, sizeof(bytes), &size) != FVM_S_OK ||
        fvm_sstream_init(&decoder, input, sizeof(input), sizeof(input)) != FVM_S_OK)
    {
        exit(3);
    }
    append(0, bytes, size);
    append(5, "other", 5);
    static const char ui[] = "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
    append(17, ui, sizeof(ui) - 1);
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


/* EN/RU: Append two complete records after host setup; selected queue will block dispatcher at second.
 * Добавить две полные записи после host setup; выбранная очередь задержит dispatcher на второй.
 */
static int command(const FVM_SRecord *record)
{
    if (record->tag != 102 || record->size != 4 || memcmp(record->payload, "fill", 4))
    {
        return 0;
    }
    unsigned queue = STYX_RETIREMENT_CASE % 3; /* General/UI/typed fixture backpressure. */
    static const char ui[] = "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
#ifdef STYX_DYNAMIC_TEST
    for (unsigned i = 0; i < 512; i++)
    {
        unsigned char value[] = {0, 0, (unsigned char)(i >> 8), (unsigned char)i}; /* Independent FIFO oracle. */
        append(queue == 0 ? 9 : queue == 1 ? 17 : 23, queue == 1 ? (const void *)ui : value,
            queue == 1 ? sizeof(ui) - 1 : sizeof(value));
    }
    unsigned char service[] = {0xaa, 0, 128, 255}; /* Canonical version42 then opaque fixture service body. */
    for (unsigned i = 0; i < 64; i++)
    {
        append(0, service, sizeof(service));
    }
    FVM_SBinding late = {31, 1, 9, "test.late"}; /* Late association must apply past application backlog. */
    unsigned char declaration[64]; /* Original binding codec output. */
    size_t size; /* Exact body extent. */
    if (fvm_sbindings_payload_write(&late, 1, declaration, sizeof(declaration), &size) != FVM_S_OK)
    {
        exit(3);
    }
    append(0, declaration, size);
    append(31, "tail", 4);
    append(9, "done", 4);
    publish();
    return 1;
#else
    unsigned copies = 2; /* Two records fill the route, additional link-test records also fill raw RX. */
#ifdef STYX_CONTROL_LINK_TEST
    copies = 6;
#endif
    for (unsigned i = 0; i < copies; i++)
    {
        append(queue == 0 ? 9 : queue == 1 ? 17 : 23, queue == 1 ? ui : "held", queue == 1 ? sizeof(ui) - 1 : 4);
    }
    publish();
    return 1;
#endif
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
