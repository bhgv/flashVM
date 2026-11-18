/* Copyright (c) 2026 bhgv
 * EN: Original libstyx peer offering independent typed, UI and unknown S-record fixtures.
 * RU: Исходный libstyx peer с независимыми типизированными, UI и неизвестными S-записями.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#include "flashvm/sbindings.h"
#ifndef STYX_TYPED_CASE
#define STYX_TYPED_CASE 0 /* Full UI and typed delivery. */
#endif
static unsigned char offered[2048]; /* Server-owner encoded RX stream, including appended receipt. */
static size_t offered_size; /* Server-owner published stream length. */
static unsigned char received[256]; /* Server-owner TX decoder storage, at least the record limit. */
static FVM_SStream decoder; /* Server-owner fragmented outgoing record decoder. */
static StyxRead *pending; /* Server-owner deferred input, released once on cancellation/completion. */
static size_t position; /* Confirmed incoming byte offset. */
static unsigned messages; /* Fully validated outgoing record count. */


/* EN/RU: Append one independent fixture record; borrow bytes through encoding, fatal on overflow.
 * Добавить одну эталонную запись, bytes заимствован до кодирования; overflow завершает fixture.
 */
static void append(unsigned long tag, const void *bytes, size_t size)
{
    size_t encoded; /* Complete record size after successful encoding. */
    if (fvm_srecord_write(tag, bytes, size, offered + offered_size,
        sizeof(offered) - offered_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += encoded;
}


/* EN/RU: Append validated declaration payload; borrowed entries, no production schema assignment.
 * Добавить объявление тестовых типов, entries заимствован; production-схемы не назначаются.
 */
static void declare(const FVM_SBinding *entries, size_t count)
{
    unsigned char bytes[512]; /* Temporary tag-zero body. */
    size_t size; /* Exact encoded payload length. */
    if (fvm_sbindings_payload_write(entries, count, bytes, sizeof(bytes), &size) != FVM_S_OK)
    {
        exit(3);
    }
    append(0, bytes, size);
}


/* EN/RU: Build the fixture only once on the original server owner; late bindings do not rewrite history.
 * Собрать fixture один раз на исходном server owner; поздние bindings не меняют прежние записи.
 */
static void initialize(void)
{
    if (offered_size)
    {
        return;
    }
    FVM_SBinding bindings[] = { /* Immutable fixture-only associations, unrelated to TX grants. */
        {5, 1, 11, "test.opaque"},
        {6, 1, 11, "test.opaque"},
        {17, 1, 13, "test.ide.fvd1"},
        {24, 2, 9, "test.file"},
        {23, 1, 9, "test.file"}
    };
    declare(bindings, STYX_TYPED_CASE == 1 ? 4 : 5);
    static const unsigned char binary[] = {0, 128, 255}; /* Independent exact binary values. */
    append(5, binary, sizeof(binary));
    append(23, "part", 4);
    append(6, nil, 0);
    static const char ui[] = "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
    append(17, ui, sizeof(ui) - 1);
    if (STYX_TYPED_CASE == 1)
    {
        declare(bindings + 4, 1);
    }
    if (STYX_TYPED_CASE == 5)
    {
        FVM_SBinding conflict = {5, 1, 9, "test.file"}; /* Illegal immutable-tag rebind. */
        declare(&conflict, 1);
    }
    append(23, "next", 4);
    append(5, "tail", 4);
    append(9, nil, 0);
    append(24, "vtwo", 4);
    if (fvm_sstream_init(&decoder, received, sizeof(received), 256) != FVM_S_OK)
    {
        exit(3);
    }
}


/* EN/RU: Retire the original deferred handle once; no fixture storage is freed here.
 * Завершить исходный deferred handle один раз, память fixture здесь не освобождается.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *request = pending; /* Handle retained by the original libstyx request. */
    pending = nil;
    styxreadfree(request);
}


/* EN/RU: Borrow request buffer/count, return at most three bytes; idle uses real deferred Tread.
 * Заимствовать buffer/count, вернуть максимум три байта; при idle настоящий deferred Tread.
 */
char *terminalread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    initialize();
    if (offset < 0 || (uvlong)offset > offered_size)
    {
        return Ebadarg;
    }
    size_t size = offered_size - (size_t)offset; /* Current available prefix. */
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


/* EN/RU: Append a receipt after exact outgoing validation, completing any real deferred read.
 * Добавить квитанцию после точной проверки TX, завершить уже ожидающий настоящий deferred read.
 */
static void acknowledge(void)
{
    append(9, "done", 4);
    if (pending)
    {
        vlong offset = styxreadoffset(pending); /* Handle valid until styxreadreply invokes release. */
        ulong count = styxreadlimit(pending); /* Original peer buffer capacity. */
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


/* EN/RU: Borrow native bytes/count; acknowledge two-byte prefixes, validate one independent TX frame.
 * Заимствовать bytes/count, подтверждать по два байта; проверить один независимый TX-эталон.
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
    size_t at = 0; /* Consumed prefix of this original Twrite. */
    while (at < *count)
    {
        FVM_SRecord record; /* Borrowed complete decoder output. */
        size_t used = 0; /* Exact consumed prefix. */
        FVM_SResult status = fvm_sstream_push(&decoder, bytes + at, *count - at, &record, &used);
        at += used;
        if (status == FVM_S_OK)
        {
            static const unsigned char expected[] = {0, 1, 128, 255, 0}; /* No text coercion. */
            if (messages || record.tag != 101 || record.size != sizeof(expected) ||
                memcmp(record.payload, expected, sizeof(expected)))
            {
                return Ebadarg;
            }
            messages++;
            acknowledge();
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
