/* Copyright (c) 2026 bhgv
 * EN: Original libstyx peer echoes checked publisher S-records into an independently routed input.
 * RU: Исходный libstyx peer отражает проверенные S-records publisher в отдельно routed input.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#include "announcement.h"
static unsigned char offered[4096]; /* Original server-owner offered S-stream. */
static size_t offered_size; /* Encoded offered bytes, including appended fixture updates. */
static unsigned char input[256]; /* Server-owner command decoder storage. */
static FVM_SStream decoder; /* Server-owner exact fragmented S-record decoder. */
static StyxRead *pending; /* Original deferred idle Tread, released once on owner. */
static size_t position; /* Confirmed incoming write extent. */
static FVM_SBindings bindings; /* Outgoing declarations validated independently of RX. */
static FVM_SBinding slots[2]; /* Two explicitly reserved publisher tags. */
static unsigned long long last[2]; /* Monotonic oracle per publisher, server owner only. */
static unsigned kinds[2]; /* Same-generation contradictions are forbidden. */


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
        {5, 1, 11, "test.opaque"}, {17, 1, 13, "test.ide.fvd1"}
    };
    unsigned char bytes[256]; /* Temporary declaration body. */
    size_t size; /* Exact encoded body extent. */
    if (fvm_sbindings_payload_write(initial, 2, bytes, sizeof(bytes), &size) != FVM_S_OK ||
        fvm_sstream_init(&decoder, input, sizeof(input), sizeof(input)) != FVM_S_OK ||
        fvm_sbindings_init(&bindings, slots, 2) != FVM_S_OK)
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


/* EN/RU: Echo whole checked frames only in this fixture; production hosts grant TX separately.
 * Test echo не задаёт production grant: host отдельно согласует TX; raw bytes не вставляются.
 */
static int command(const FVM_SRecord *record)
{
    if (record->tag == 102 && record->size == 5 && !memcmp(record->payload, "alive", 5))
    {
        append(9, "alive", 5);
    }
    else if (!record->tag)
    {
        unsigned char frame[512]; /* Reconstruct one checked declaration for the shared registry. */
        size_t size; /* Full frame extent. */
        size_t used; /* Exact registry consumption. */
        if (fvm_srecord_write(0, record->payload, record->size, frame, sizeof(frame), &size) != FVM_S_OK ||
            fvm_sbindings_apply(&bindings, frame, size, &used) != FVM_S_OK || used != size)
        {
            return 0;
        }
        append(0, record->payload, record->size);
    }
    else
    {
        FVM_SBinding binding; /* Copy of exact outgoing declaration. */
        unsigned state; /* Existing READY/CLOSED payload kind. */
        unsigned long long generation; /* Independent monotonic wire oracle. */
        if ((record->tag != 23 && record->tag != 24) ||
            fvm_sbindings_lookup(&bindings, record->tag, &binding) != FVM_S_OK ||
            binding.version != MOVIE_OFFER_VERSION || strcmp(binding.type, MOVIE_OFFER_TYPE) ||
            !movie_offer_parse(record->payload, record->size, &state, &generation))
        {
            return 0;
        }
        unsigned index = record->tag - 23; /* Only the two reserved fixture tags. */
        if (generation < last[index] || (generation == last[index] && kinds[index] != state))
        {
            return 0;
        }
        last[index] = generation;
        kinds[index] = state;
        append(record->tag, record->payload, record->size);
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
