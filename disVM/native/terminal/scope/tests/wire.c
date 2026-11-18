/* Copyright (c) 2026 bhgv
 * EN: Independent original-Styx nested-envelope fixture, colliding local tags and validated outgoing wraps.
 * RU: Независимый original-Styx nested-envelope fixture, одинаковые local tags и проверка outgoing wraps.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
static unsigned char offered[65536]; /* Original server-owner output, includes actual full S-records. */
static size_t offered_size;          /* Encoded offered extent. */
static unsigned char input[8192];    /* One incoming original S-record decoder buffer. */
static FVM_SStream decoder;          /* Original fragmented command/TX decoder. */
static StyxRead *pending;            /* Original deferred idle Tread, released on server owner. */
static size_t position;              /* Exact acknowledged write prefix. */


/* EN/RU: Encode one full frame into bounded output; borrowed inputs, return exact extent or fatal test failure.
 * Кодировать полный frame в bounded output, inputs borrowed, exact extent либо fatal test failure.
 */
static size_t frame(unsigned long tag, const void *bytes, size_t size, void *out, size_t capacity)
{
    size_t used; /* Exact original codec output extent. */
    if (fvm_srecord_write(tag, bytes, size, out, capacity, &used) != FVM_S_OK)
    {
        exit(3);
    }
    return used;
}


/* EN/RU: Append one complete outer record to original server output, preserving existing unread bytes.
 * Добавить полный outer record в server output, сохраняя непрочитанные bytes.
 */
static void append(unsigned long tag, const void *bytes, size_t size)
{
    offered_size += frame(tag, bytes, size, offered + offered_size, sizeof(offered) - offered_size);
}


/* EN/RU: Wrap one local client record in an explicit outer tag; no production tag identities assigned here.
 * Обернуть local client record в явный fixture outer tag; production tag identities здесь не назначаются.
 */
static void client(unsigned long outer, unsigned long inner, const void *bytes, size_t size)
{
    unsigned char encoded[1024]; /* Independent complete inner frame. */
    size_t used = frame(inner, bytes, size, encoded, sizeof(encoded)); /* Golden hierarchy uses original codec. */
    append(outer, encoded, used);
}


/* EN/RU: Release one original deferred read handle after reply/flush/connection retirement.
 * Освободить original deferred read handle после reply/flush/connection retirement.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Consume the original retained server handle exactly once. */
    pending = nil;
    styxreadfree(read);
}


/* EN/RU: Reply to a real pending Tread after appending input, with original three-byte short prefixes.
 * Ответить pending Tread после добавления input, сохраняя prefixes по три bytes.
 */
static void publish(void)
{
    if (pending)
    {
        vlong offset = styxreadoffset(pending); /* Borrowed through reply/release. */
        ulong count = styxreadlimit(pending); /* Original caller capacity. */
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


/* EN/RU: Initialize only a root-native ready record; no application bytes before native binding setup.
 * Инициализировать лишь root-native ready record, без application bytes до native bindings.
 */
static void initialize(void)
{
    if (!offered_size)
    {
        append(0, "ready", 5);
        if (fvm_sstream_init(&decoder, input, sizeof(input), 4096) != FVM_S_OK)
        {
            exit(3);
        }
    }
}


/* EN/RU: Original server read callback; precise three-byte prefix or original deferred idle Tread.
 * Original server read callback: prefix по три bytes либо original deferred idle Tread.
 */
char *terminalread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    initialize();
    if (offset < 0 || (uvlong)offset > offered_size)
    {
        return Ebadarg;
    }
    size_t available = offered_size - (size_t)offset; /* Real unread encoded bytes. */
    if (!available)
    {
        if (pending)
        {
            return Eperm;
        }
        pending = styxdeferread(server, nil, release);
        *count = 0;
        return pending ? nil : Enomem;
    }
    if (available > *count)
    {
        available = *count;
    }
    if (available > 3)
    {
        available = 3;
    }
    memmove(bytes, offered + offset, available);
    *count = available;
    return nil;
}


/* EN/RU: First publication includes same local7 in unrelated clients and groups, plus opaque leaf bytes.
 * Первая publication включает local7 разных clients/groups и opaque leaf bytes.
 */
static void fill(void)
{
    if (STYX_SCOPE_CASE == 11)
    {
        unsigned char body[128] = {0}; /* Inner payload chosen to make a full encoded frame exactly128bytes. */
        unsigned char encoded[256]; /* Control accepts this outer payload but scoped body limit must charge tag too. */
        for (size_t size = 0; size < sizeof(body); size++)
        {
            size_t used = frame(7, body, size, encoded, sizeof(encoded));
            if (used == 128)
            {
                append(11, encoded, used);
                return;
            }
        }
        exit(3);
    }
    unsigned char records[1024]; /* One complete multi-record client envelope. */
    size_t used = frame(7, "alpha", 5, records, sizeof(records)); /* First staged record. */
    used += frame(0, "local-zero", 10, records + used, sizeof(records) - used);
    if (STYX_SCOPE_CASE == 1)
    {
        records[used - 1] ^= 1; /* Corrupt later frame; earlier staged record must never leak. */
        append(11, records, used);
        return;
    }
    if (STYX_SCOPE_CASE == 3)
    {
        append(99, records, used); /* Unknown native outer grant, never an application-local tag. */
        return;
    }
    unsigned char opaque[64]; /* Valid-looking S-record retained as opaque leaf payload. */
    size_t opaque_size = frame(0, "not-system", 10, opaque, sizeof(opaque)); /* Cannot escape client's scope. */
    used += frame(8, opaque, opaque_size, records + used, sizeof(records) - used);
    append(11, records, used);
    client(12, 7, "beta", 4);
    unsigned char inner[512]; /* Group payload with its own service and a child envelope. */
    size_t size = frame(7, "group", 5, opaque, sizeof(opaque)); /* Client C reuses local7. */
    size_t group_size = frame(0, "group-system", 12, inner, sizeof(inner)); /* Native group service only. */
    group_size += frame(11, opaque, size, inner + group_size, sizeof(inner) - group_size);
    if (STYX_SCOPE_CASE == 6)
    {
        inner[group_size - 1] ^= 1; /* Reject complete group transaction, including earlier group service. */
    }
    append(20, inner, group_size);
    for (unsigned i = 0; i < 512; i++)
    {
        unsigned char value[] = {0, 0, (unsigned char)(i >> 8), (unsigned char)i}; /* Independent FIFO sequence. */
        client(11, 7, value, sizeof(value));
    }
    append(0, "system", 6);
    append(0, "done", 4);
}


/* EN/RU: Decode exactly one expected nested record; borrowed view, fatal mismatch in independent server oracle.
 * Прочитать ровно ожидаемый nested record; borrowed view, fatal mismatch independent server oracle.
 */
static FVM_SRecord record(const void *bytes, size_t size, unsigned long tag)
{
    FVM_SRecord view; /* Borrowed independent codec view. */
    size_t used; /* Exact nested extent. */
    if (fvm_srecord_read(bytes, size, 4096, &view, &used) != FVM_S_OK || used != size || view.tag != tag)
    {
        exit(3);
    }
    return view;
}


#ifdef STYX_SCOPE_OPERATION_TEST


/* EN/RU: Independent operation fixture handles its root-service/native data commands, 1 handled/0 unknown/-1 invalid.
 * Independent operation fixture обрабатывает root-service/native data commands, 1/0/-1.
 */
extern int scopeoperationcommand(const FVM_SRecord *);


/* EN/RU: Append original encoded output for the operation fixture on its sole server owner.
 * Добавить original encoded output operation fixture на sole server owner.
 */
void scopeoperationoffer(unsigned long tag, const void *bytes, size_t size)
{
    append(tag, bytes, size);
}


/* EN/RU: Append one client-local message inside an explicit fixture outer tag, same original codec.
 * Добавить client-local message в fixture outer tag исходным кодеком.
 */
void scopeoperationclient(unsigned long outer, unsigned long inner, const void *bytes, size_t size)
{
    client(outer, inner, bytes, size);
}
#endif


/* EN/RU: Verify native fixture control or complete client TX envelope, append only root-native receipts.
 * Проверить fixture control либо полный client TX envelope, receipts лишь root-native.
 */
static int command(const FVM_SRecord *message)
{
#ifdef STYX_SCOPE_OPERATION_TEST
    int operation = scopeoperationcommand(message); /* Fixture-only service schemas, not production numeric IDs. */
    if (operation)
    {
        if (operation > 0)
        {
            publish();
        }
        return operation > 0;
    }
#endif
    if (message->tag == 102 && message->size == 4 && !memcmp(message->payload, "fill", 4))
    {
        fill();
    }
    else if (message->tag == 102 && message->size == 4 && !memcmp(message->payload, "next", 4))
    {
        client(11, 7, "new-B", 5);
        client(12, 7, "new-A", 5);
        append(0, "after", 5);
    }
    else if (message->tag == 31)
    {
        FVM_SRecord local = record(message->payload, message->size, 7); /* Explicit TX tag differs from RX. */
        if (local.size != 9 || memcmp(local.payload, "client-tx", 9))
        {
            return 0;
        }
        append(0, "tx-ack", 6);
    }
    else if (message->tag == 40)
    {
        FVM_SRecord group = record(message->payload, message->size, 41); /* Explicit group-local TX path. */
        FVM_SRecord local = record(group.payload, group.size, 0); /* Client-local zero, never transport control. */
        if (local.size != 13 || memcmp(local.payload, "child-control", 13))
        {
            return 0;
        }
        append(0, "nested-tx", 9);
    }
    else
    {
        return 0;
    }
    publish();
    return 1;
}


/* EN/RU: Original server write callback acknowledges two-byte prefixes and decodes actual full S-records.
 * Original server write callback: ACK по два bytes и decode actual complete S-records.
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
    size_t at = 0; /* Confirmed prefix within this actual Twrite. */
    while (at < *count)
    {
        FVM_SRecord message; /* Borrowed complete original decoder view. */
        size_t used = 0; /* Exact consumed prefix. */
        FVM_SResult status = fvm_sstream_push(&decoder, bytes + at, *count - at, &message, &used);
        at += used;
        if (status == FVM_S_OK)
        {
            if (!command(&message))
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
