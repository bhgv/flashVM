/* Copyright (c) 2026 bhgv
 * EN: Original libstyx terminal fixture with fragmented binary records and native deferred idle input.
 * RU: Original libstyx terminal fixture с fragmented binary records и native deferred idle input.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#include "flashvm/sbindings.h"
#if defined(STYX_UI_BRIDGE_CASE) && (STYX_UI_BRIDGE_CASE == 2 || STYX_UI_BRIDGE_CASE == 9)
static unsigned char offered[8192]; /* Repeated UI input to fill the real local channel. */
#else
static unsigned char offered[256]; /* Owner-built encoded input stream; owner may append a test receipt. */
#endif
static size_t offered_size; /* Total offered bytes. */
static unsigned char received[512]; /* Owner-only transmit decoder storage. */
static FVM_SStream decoder; /* Owner-only across exact write prefixes. */
static StyxRead *pending; /* Owner-only original deferred read, released on Tflush/clunk/end. */
static size_t position; /* Confirmed peer write offset. */
static unsigned messages; /* Complete records received, owner-only. */


/* EN/RU: Build fixtures once on original server owner; no new production event schemas.
 * Построить fixture один раз на server owner; без новых production schemas. */
static void initialize(void)
{
    if (offered_size)
    {
        return;
    }
    FVM_SBinding binding[] = { /* Fixture-only immutable RX identities. */
        {5, 1, 11, "test.opaque"},
        {17, 1, 13, "test.ide.fvd1"}
    };
    size_t binding_count = 2; /* Normally declare both identities before payloads. */
#if defined(STYX_ROUTE_CASE) && STYX_ROUTE_CASE == 5
    binding_count = 1; /* The UI identity is deliberately announced later. */
#endif
    unsigned char declaration[128]; /* Owned temporary control payload. */
    size_t size; /* Encoded declaration payload bytes. */
    size_t encoded; /* Complete S-record bytes. */
    if (fvm_sbindings_payload_write(binding, binding_count, declaration,
        sizeof(declaration), &size) != FVM_S_OK ||
        fvm_srecord_write(0, declaration, size, offered, sizeof(offered), &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size = encoded;
    unsigned char binary[] = {0, 128, 255};
    if (fvm_srecord_write(5, binary, sizeof(binary), offered+offered_size, sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
    if (fvm_srecord_write(9, nil, 0, offered+offered_size, sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
    /* EN/RU: Independent fvd1 golden payload; tag17 is a fixture-only UI binding.
     * Независимый эталон fvd1; tag17 — только тестовая UI-привязка. */
#if defined(STYX_ROUTE_CASE) && STYX_ROUTE_CASE == 3
    /* EN/RU: Illegal immutable-tag rebind must not replace an admitted UI route.
     * Конфликтующее объявление не должно заменять уже принятую UI-привязку. */
    FVM_SBinding conflict = {17, 1, 10, "test.other"};
    if (fvm_sbindings_payload_write(&conflict, 1, declaration, sizeof(declaration), &size) != FVM_S_OK ||
        fvm_srecord_write(0, declaration, size, offered+offered_size,
            sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
#endif
#if (defined(STYX_ROUTE_CASE) && STYX_ROUTE_CASE == 2) || \
    (defined(STYX_UI_BRIDGE_CASE) && STYX_UI_BRIDGE_CASE == 5)
    static const char ui[] = "bad-ui-frame";
#else
    static const char ui[] = "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
#endif
#if defined(STYX_ROUTE_CASE) && STYX_ROUTE_CASE == 5
    /* EN/RU: Preserve an unbound record, then announce its UI identity on the live connection.
     * Сначала неизвестная запись, затем объявление её UI-типа на живом соединении. */
    if (fvm_srecord_write(17, ui, sizeof(ui)-1, offered+offered_size,
        sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
    if (fvm_sbindings_payload_write(binding+1, 1, declaration, sizeof(declaration), &size) != FVM_S_OK ||
        fvm_srecord_write(0, declaration, size, offered+offered_size,
            sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
#endif
    if (fvm_srecord_write(17, ui, sizeof(ui)-1, offered+offered_size,
        sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
#if defined(STYX_ROUTE_CASE) && STYX_ROUTE_CASE == 6
    /* EN/RU: Second UI item must backpressure the one-slot mailbox, not batch or overwrite it.
     * Вторая UI-запись ожидает место, не объединяется и не заменяет первую. */
    if (fvm_srecord_write(17, ui, sizeof(ui)-1, offered+offered_size,
        sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
#endif
#if defined(STYX_UI_BRIDGE_CASE) && (STYX_UI_BRIDGE_CASE == 2 || STYX_UI_BRIDGE_CASE == 9)
    for (unsigned i = 0; i < 40; i++)
    {
        if (fvm_srecord_write(17, ui, sizeof(ui)-1, offered+offered_size,
            sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
        {
            exit(3);
        }
        offered_size += encoded;
    }
#endif
    fvm_sstream_init(&decoder, received, sizeof(received), 256);
}


/* EN/RU: Original deferred-read retirement, exactly once on native owner. / Original deferred-read retirement. */
static void release(void *unused)
{
    USED(unused);
    StyxRead *request = pending;
    pending = nil;
    styxreadfree(request);
}


/* EN/RU: Return at most three fixture bytes or an actual idle deferred Tread, never false EOF.
 * Вернуть <=3 bytes либо настоящий deferred Tread при idle, не false EOF. */
char *terminalread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    initialize();
    if (offset < 0 || (uvlong)offset > offered_size)
    {
        return Ebadarg;
    }
    size_t size = offered_size-(size_t)offset;
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
    memmove(bytes, offered+offset, size);
    *count = size;
    return nil;
}


#ifdef STYX_UI_BRIDGE_CASE
/* EN/RU: Append a test-only completion receipt after all reply bytes were validated.
 * Complete an already deferred read on its real server owner, clipping the prefix to three bytes.
 * Добавить тестовую квитанцию после проверки всех ответов, завершить deferred read на owner.
 */
static void bridgeack(void)
{
    size_t encoded; /* Complete receipt frame bytes appended to the offered stream. */
    if (fvm_srecord_write(9, "done", 4, offered+offered_size,
        sizeof(offered)-offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
    if (pending)
    {
        vlong offset = styxreadoffset(pending); /* Deferred handle is owned by this native owner. */
        ulong count = styxreadlimit(pending); /* Exact remote buffer capacity. */
        if (offset < 0 || (uvlong)offset >= offered_size)
        {
            exit(3);
        }
        if (count > offered_size-(size_t)offset)
        {
            count = offered_size-(size_t)offset;
        }
        if (count > 3)
        {
            count = 3;
        }
        /* release() retires the caller handle synchronously; never access it after this call. */
        if (!styxreadreply(pending, offered+offset, count, nil))
        {
            exit(3);
        }
    }
}
#endif


/* EN/RU: Clip writes to two bytes and validate record sequence across all confirmed prefixes.
 * Записывать <=2 bytes, проверять records через все confirmed prefixes, без interleave/replay. */
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
    size_t at = 0;
    while (at < *count)
    {
        FVM_SRecord record;
        size_t used = 0;
        FVM_SResult status = fvm_sstream_push(&decoder, bytes+at, *count-at, &record, &used);
        at += used;
        if (status == FVM_S_OK)
        {
            unsigned maximum = 3; /* Complete control/binary/reply sequence. */
#if defined(STYX_UI_BRIDGE_CASE) && STYX_UI_BRIDGE_CASE == 4
            maximum = 10; /* Eight independently relayed replies after two setup records. */
#endif
            if ((messages == 0 && (record.tag != 0 || record.size != 2)) ||
                (messages == 1 && (record.tag != 101 || record.size != 240)) || messages >= maximum)
            {
                return Ebadarg;
            }
            if (messages == 1)
            {
                for (size_t i = 0; i < record.size; i++)
                {
                    if (record.payload[i] != (unsigned char)i)
                    {
                        return Ebadarg;
                    }
                }
            }
            if (messages >= 2)
            {
                /* EN/RU: Compare exact independent reply bytes, not this client's encoder.
                 * Сравнить точные байты ответа с независимым эталоном. */
                static const char reply[] = "fvd1:000000020000002b0000004d000000000000000000000000reply";
                if (record.tag != 17 || record.size != sizeof(reply)-1 ||
                    memcmp(record.payload, reply, sizeof(reply)-1))
                {
                    return Ebadarg;
                }
            }
            messages++;
#ifdef STYX_UI_BRIDGE_CASE
            if (messages == maximum)
            {
                bridgeack();
            }
#endif
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
