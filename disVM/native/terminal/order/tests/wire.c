/* Copyright (c) 2026 bhgv
 * EN: Original Styx peer emits explicitly prefixed packets, deferring idle reads until a native release command.
 * RU: Исходный Styx peer выдаёт пакеты с префиксами, откладывая пустое чтение до команды release.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sorder.h"
static unsigned char offered[2048]; /* Original server-owner output log. */
static size_t offered_size; /* Published bytes, offsets remain original Styx offsets. */
static StyxRead *pending; /* One original deferred read handle. */


/* EN/RU: Append one packet at selected tag/ordinal; fixture owns bytes until teardown.
 * Добавить пакет выбранного tag/ordinal; fixture владеет байтами до teardown.
 */
static void append(unsigned long tag, unsigned long low, const char *body)
{
    FVM_SOrdinal ordinal = {0, low}; /* Exact selected fixture number. */
    size_t used; /* Appended packet extent. */
    if (fvm_sordered_write(&ordinal, tag, body, strlen(body), offered + offered_size,
        sizeof(offered) - offered_size, &used) != FVM_S_OK)
    {
        exit(3);
    }
    offered_size += used;
}


/* EN/RU: Consume retained original read handle on reply/flush/disconnect, only server owner calls.
 * Освободить исходный read handle при reply/flush/disconnect, только на server owner.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Owned handle transferred before free. */
    pending = nil;
    styxreadfree(read);
}


/* EN/RU: Serve at most three bytes of actual packets, never report idle as EOF.
 * Выдать не более трёх байт настоящих пакетов, пустое ожидание не является EOF.
 */
char *terminalread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    if (!offered_size)
    {
        append(7, 2, "two");
        append(9, 0, "nine");
    }
    if (offset < 0 || (uvlong)offset > offered_size)
    {
        return Ebadarg;
    }
    if ((uvlong)offset == offered_size)
    {
        if (pending)
        {
            return Eperm;
        }
        pending = styxdeferread(server, nil, release);
        *count = 0;
        return pending ? nil : Enomem;
    }
    if (*count > offered_size - (size_t)offset)
    {
        *count = offered_size - (size_t)offset;
    }
    if (*count > 3)
    {
        *count = 3;
    }
    memmove(bytes, offered + offset, *count);
    return nil;
}


/* EN/RU: Fixture-only plain S tag0 release command, not a production reset/negotiation schema.
 * Только тестовая plain S-команда tag0 release, не production-формат reset/negotiation.
 */
char *terminalwrite(char *bytes, ulong *count, vlong offset)
{
    FVM_SRecord record; /* Verified borrowed original command. */
    size_t used; /* One exact command extent. */
    USED(offset);
    if (fvm_srecord_read(bytes, *count, 128, &record, &used) != FVM_S_OK || used != *count ||
        record.tag || record.size != 7 || memcmp(record.payload, "release", 7))
    {
        return Ebadarg;
    }
    append(7, 0, "zero");
    append(7, 1, "one");
    if (pending)
    {
        vlong at = styxreadoffset(pending); /* Original pending offset. */
        ulong size = styxreadlimit(pending); /* Original requested capacity. */
        if (size > 3)
        {
            size = 3;
        }
        if (at < 0 || (uvlong)at + size > offered_size || !styxreadreply(pending, offered + at, size, nil))
        {
            exit(3);
        }
    }
    return nil;
}
