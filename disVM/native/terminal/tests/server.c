/* Copyright (c) 2026 bhgv
 * EN: Original libstyx terminal fixture with fragmented binary records and native deferred idle input.
 * RU: Original libstyx terminal fixture с fragmented binary records и native deferred idle input.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#include "flashvm/sbindings.h"
static unsigned char offered[256]; /* Owner-built immutable encoded input stream. */
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
    FVM_SBinding binding = {5, 1, 11, "test.opaque"};
    unsigned char declaration[128];
    size_t size, encoded;
    if (fvm_sbindings_payload_write(&binding, 1, declaration, sizeof(declaration), &size) != FVM_S_OK ||
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
            if ((messages == 0 && (record.tag != 0 || record.size != 2)) ||
                (messages == 1 && (record.tag != 101 || record.size != 240)) || messages > 1)
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
            messages++;
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
