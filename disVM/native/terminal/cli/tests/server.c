/* Copyright (c) 2026 bhgv
 * EN: Original Styx/P9MX peer oracle for executable CLI application records, renewal and opaque root services.
 * RU: Исходный Styx/P9MX peer для CLI: клиентские записи, сброс счётчика и закрытые root-службы.
 */
#include <stdio.h>
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "mux/bidir.h"
#include "drivers/posix/fd.h"
#include "flashvm/sordctl.h"
#include <stdlib.h>
static Styxserver server; /* Sole original server owner is main. */
static unsigned char output[262144]; /* Published original byte log. */
static size_t extent; /* Published prefix, stable offsets. */
static unsigned char storage[4096]; /* Incoming ordinary stream decoder storage. */
static FVM_SStream decoder; /* Original fragmented outer decoder. */
static size_t written; /* Exact Twrite acknowledged prefix. */
static StyxRead *pending; /* Original deferred Tread handle. */
static int published; /* Initial data emitted once. */
static int renewed; /* Selected RX reset ACK observed. */
static int echo; /* Host data request retained until reset ACK. */
static const char *mode; /* Immutable fixture scenario. */


/* EN/RU: Explicit independent native wire contract, never optimized away.
 * Явная независимая проверка протокола, не удаляемая оптимизатором.
 */
static void check(int valid)
{
    if (!valid)
    {
        fputs("CLI original-Styx peer mismatch\n", stderr);
        exit(3);
    }
}


/* EN/RU: Append one ordinary outer record, output owner retains every byte until teardown.
 * Добавить обычную внешнюю запись, output-owner сохраняет все байты до teardown.
 */
static void append(ulong tag, const void *bytes, size_t size)
{
    size_t used; /* Complete original frame extent. */
    check(fvm_srecord_write(tag, bytes, size, output + extent, sizeof(output) - extent, &used) == FVM_S_OK);
    extent += used;
}


/* EN/RU: Append one prefixed inner packet on negotiated lane91, no native generation field.
 * Добавить внутренний пакет с префиксом на согласованной линии91, без native-поколения.
 */
static void data(ulong tag, ulong number, const void *bytes, size_t size)
{
    unsigned char encoded[512]; /* Complete prefix and original inner record. */
    FVM_SOrdinal ordinal = {0, number}; /* Explicit portable number. */
    size_t used; /* Exact packet extent. */
    check(fvm_sordered_write(&ordinal, tag, bytes, size, encoded, sizeof(encoded), &used) == FVM_S_OK);
    append(91, encoded, used);
}


/* EN/RU: Wrap one named native service in outer root0; never stdout application data.
 * Обернуть именованную native-службу во внешний root0; это не stdout-данные клиента.
 */
static void service(FVM_SOrderControl *message)
{
    unsigned char bytes[FVM_SORDER_CTL_MAX]; /* Named body. */
    size_t used; /* Exact body extent. */
    check(fvm_sorder_control_write(message, bytes, sizeof(bytes), &used) == FVM_S_OK);
    append(0, bytes, used);
}


/* EN/RU: Publish disorder, binary and local0 data; request selected renewal only in full echo scenario.
 * Выдать переставленные, двоичные и local0-данные; запросить сброс лишь в полном echo-сценарии.
 */
static void initial(void)
{
    if (published)
    {
        return;
    }
    published = 1;
    if (!strcmp(mode, "tiny"))
    {
        data(0, 0, "", 0);
        return;
    }
    if (!strcmp(mode, "eof-single"))
    {
        data(7, 0, "last", 4);
        return;
    }
    if (!strcmp(mode, "flood"))
    {
        char payload[200]; /* Fixture-only application pressure, far larger than bounded stdout pipe capacity. */
        memset(payload, 'F', sizeof(payload));
        for (ulong i = 0; i < 512; i++)
        {
            data(7, i, payload, sizeof(payload));
        }
        return;
    }
    data(7, 2, "two", 3);
    data(9, 0, "\0\377", 2);
    data(0, 0, "client-zero", 11);
    data(7, 0, "zero", 4);
    data(7, 1, "one", 3);
    append(0, "private-root", 12);
    if (!strcmp(mode, "normal"))
    {
        FVM_SOrderControl reset = {0}; /* Peer TX tag7 boundary2. */
        reset.command = FVM_SORDER_RENEW;
        reset.operation.low = 1;
        reset.lane = 91;
        reset.tag = 7;
        reset.boundary.low = 2;
        service(&reset);
    }
}


/* EN/RU: Independent peer command verification, same application-local0 never becomes root service.
 * Независимая проверка peer-команд; клиентский0 никогда не становится root-службой.
 */
static void command(const FVM_SRecord *record)
{
    if (!record->tag)
    {
        FVM_SOrderControl message; /* Checked native service. */
        check(fvm_sorder_control_read(record->payload, record->size, &message) == FVM_S_OK);
        if (message.command == FVM_SORDER_OFFER)
        {
            check(message.lane == 71 && message.limit == (!strcmp(mode, "tiny") ? 1 : 256));
            message.command = FVM_SORDER_ACCEPT;
            if (!strcmp(mode, "badack"))
            {
                message.lane++;
            }
            service(&message);
            message.command = FVM_SORDER_OFFER;
            message.lane = 91;
            message.window.high = 0;
            message.window.low = 8;
            service(&message);
        }
        else if (message.command == FVM_SORDER_ACCEPT)
        {
            check(message.lane == 91);
            initial();
        }
        else
        {
            check(message.command == FVM_SORDER_ACK && message.lane == 91 && message.tag == 7 && message.boundary.low == 2);
            renewed = 1;
            if (echo)
            {
                data(7, 0, "echo", 4);
            }
        }
        return;
    }
    FVM_SOrderedRecord packet; /* Verified application packet, no transport/root authority in its payload. */
    size_t used; /* Exact packet size. */
    check(record->tag == 71 && fvm_sordered_read(record->payload, record->size, 256, &packet, &used) == FVM_S_OK && used == record->size);
    check(!packet.ordinal.high && !packet.ordinal.low);
    if (packet.record.tag == 7)
    {
        check(packet.record.size == 4 && !memcmp(packet.record.payload, "echo", 4));
        echo = 1;
        if (renewed)
        {
            data(7, 0, "echo", 4);
        }
    }
    else
    {
        check(!packet.record.tag && packet.record.size == 5 && !memcmp(packet.record.payload, "hello", 5));
        data(0, 1, "hello", 5);
    }
}


/* EN/RU: Release original deferred read on reply, flush or connection retirement.
 * Освободить исходный отложенный read при reply, flush либо retirement соединения.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Transfer owned handle before disposal. */
    pending = nil;
    styxreadfree(read);
}


/* EN/RU: Original three-byte reads; EOF mode ends only after every published byte, live modes defer idle.
 * Исходные трёхбайтные чтения; EOF лишь после всех опубликованных байт, живые сценарии откладывают ожидание.
 */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    check(qid.path == 7 && offset >= 0 && (uvlong)offset <= extent);
    if ((uvlong)offset == extent)
    {
        *count = 0;
        if (published && (!strcmp(mode, "eof") || !strcmp(mode, "eof-single")))
        {
            return nil;
        }
        check(!pending);
        pending = styxdeferread(&server, nil, release);
        return pending ? nil : Enomem;
    }
    if (!strcmp(mode, "flood") && offset >= 16384)
    {
        fputs("delivered-flood\n", stderr);
        fflush(stderr);
        mode = "flood-marked";
    }
    if (*count > extent - (size_t)offset)
    {
        *count = extent - (size_t)offset;
    }
    if (*count > 3)
    {
        *count = 3;
    }
    memmove(bytes, output + offset, *count);
    return nil;
}


/* EN/RU: Original two-byte write acknowledgements, exact retained prefix and deferred-read publication.
 * Исходные двухбайтные подтверждения записи, точный prefix и публикация отложенного чтения.
 */
static char *writefile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    check(qid.path == 7 && offset >= 0 && (uvlong)offset == written);
    if (*count > 2)
    {
        *count = 2;
    }
    written += *count;
    for (size_t at = 0; at < *count;)
    {
        FVM_SRecord record; /* Borrowed ordinary original record. */
        size_t used; /* Exact consumed fragment. */
        FVM_SResult result = fvm_sstream_push(&decoder, bytes + at, *count - at, &record, &used); /* Verified framing. */
        check(used && (result == FVM_S_MORE || result == FVM_S_OK));
        at += used;
        if (result == FVM_S_OK)
        {
            command(&record);
            check(fvm_sstream_reset(&decoder) == FVM_S_OK);
        }
    }
    if (pending && (uvlong)styxreadoffset(pending) < extent)
    {
        vlong at = styxreadoffset(pending); /* Original pending read offset. */
        ulong size = styxreadlimit(pending); /* Original caller capacity. */
        if (size > extent - (size_t)at)
        {
            size = extent - (size_t)at;
        }
        if (size > 3)
        {
            size = 3;
        }
        check(styxreadreply(pending, output + at, size, nil));
    }
    return nil;
}
static Styxops operations = {.read = readfile, .write = writefile}; /* Original callbacks live through styxend. */


/* EN/RU: Own inherited private fixture carrier, serve one original session and consume every native owner.
 * Владеть унаследованным private-carrier fixture, обслужить исходную сессию и освободить native-owners.
 */
int main(int argc, char **argv)
{
    check(argc == 4);
    mode = argv[3];
    PhyBidir *mux = phybidircreate(phyposixfd(atoi(argv[1]), PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536); /* Own fixture carrier. */
    check(mux && !styxphyinit(&server, &operations, phybidirchannel(mux, (unsigned)atoi(argv[2])), 0555, 1, 2));
    check(styxaddfile(&server, Qroot, 7, "records", 0666, "fixture") != nil);
    check(fvm_sstream_init(&decoder, storage, sizeof(storage), 1024) == FVM_S_OK);
    while (!styxwait(&server) && !styxprocess(&server))
    {
    }
    styxend(&server);
    phybidirfree(mux);
    return 0;
}
