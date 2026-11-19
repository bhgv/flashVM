/* Copyright (c) 2026 bhgv
 * EN: Original Styx peer oracle, named offers, two-byte Twrite acknowledgements and deferred three-byte reads.
 * RU: Исходный Styx peer: именованные предложения, двухбайтные Twrite-ответы и отложенные трёхбайтные чтения.
 */
#include <stdio.h>
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sordctl.h"
static unsigned char output[8192]; /* Server-owner immutable published prefix. */
static size_t extent; /* Published bytes. */
static unsigned char storage[1024]; /* Original command decoder storage. */
static FVM_SStream decoder; /* Server-owner incremental ordinary outer decoder. */
static int initialized; /* Decoder setup completed. */
static size_t written; /* Exact acknowledged input prefix. */
static StyxRead *pending; /* Original deferred read, server owner only. */
static FVM_SOrderControl host_reset; /* Last observed host TX renewal. */
static FVM_SOrderControl remote_reset; /* Peer TX renewal, ACK must exactly match. */
static unsigned host_records; /* Host tag7 packet count. */
static int host_zero; /* Inner application tag0 observed, never parsed as service. */


/* EN/RU: Fatal wire contract, active in every optimization profile; no VM state.
 * Явная проверка протокола во всех профилях оптимизации; без VM-состояния.
 */
static void check(int condition)
{
    if (!condition)
    {
        fprintf(stderr, "ordered original-Styx wire mismatch, case %d\n", ORDERED_CASE);
        exit(3);
    }
}


/* EN/RU: Convert fixture uint64 scalar explicitly, not native struct layout.
 * Перевести тестовое uint64-число явно, не сериализовать native-структуру.
 */
static FVM_SOrdinal number(uvlong value)
{
    FVM_SOrdinal result = {(ulong)(value >> 32), (ulong)(value & 0xffffffffULL)}; /* Portable parts. */
    return result;
}


/* EN/RU: Append ordinary outer S-record; caller payload borrowed, output owned until teardown.
 * Добавить обычную внешнюю S-запись; payload заимствован, output принадлежит fixture до teardown.
 */
static void append(ulong tag, const void *bytes, size_t size)
{
    size_t used; /* Complete outer frame extent. */
    check(fvm_srecord_write(tag, bytes, size, output + extent, sizeof(output) - extent, &used) == FVM_S_OK);
    extent += used;
}


/* EN/RU: Encode named fixture service into root0, preserving the normal outer framing.
 * Кодировать именованную службу fixture в root0, сохраняя обычное внешнее обрамление.
 */
static void service(const FVM_SOrderControl *message)
{
    unsigned char bytes[FVM_SORDER_CTL_MAX]; /* Complete checked named body. */
    size_t used; /* Named body extent. */
    check(fvm_sorder_control_write(message, bytes, sizeof(bytes), &used) == FVM_S_OK);
    append(0, bytes, used);
}


/* EN/RU: Append one ordered inner packet on peer TX lane91, independent tag counters.
 * Добавить один внутренний пакет на peer TX-линии91, со счётчиком своего тега.
 */
static void data(ulong tag, uvlong ordinal, const char *text)
{
    unsigned char bytes[256]; /* Complete prefix plus original inner S-record. */
    FVM_SOrdinal sequence = number(ordinal); /* Explicit portable ordinal. */
    size_t used; /* Complete packet extent. */
    check(fvm_sordered_write(&sequence, tag, text, strlen(text), bytes, sizeof(bytes), &used) == FVM_S_OK);
    append(91, bytes, used);
}


/* EN/RU: Publish old RX data, then request renewal before clients consume it; other tag remains active.
 * Опубликовать старые RX-данные и запросить сброс до чтения клиентами; другой тег продолжает работу.
 */
static void incoming(void)
{
    if (ORDERED_CASE == 5)
    {
        data(7, ~0ULL, "two");
        data(9, 0, "nine");
        data(7, ~0ULL - 1, "one");
    }
    else
    {
        data(7, 2, "two");
        data(9, 0, "nine");
        data(7, 0, "zero");
        data(7, 1, "one");
    }
    data(0, 0, "local-in");
    memset(&remote_reset, 0, sizeof(remote_reset));
    remote_reset.command = FVM_SORDER_RENEW;
    remote_reset.operation = number(1);
    remote_reset.lane = 91;
    remote_reset.tag = 7;
    remote_reset.boundary = number(ORDERED_CASE == 5 ? ~0ULL : 2);
    service(&remote_reset);
    service(&remote_reset); /* Pending duplicate cannot emit an early ACK. */
    data(9, 1, "rx-live");
}


/* EN/RU: Validate peer control independently from production coordinator, including exact boundaries.
 * Проверить peer-команды отдельно от production-координатора, включая точные границы.
 */
static void control(const FVM_SOrderControl *message)
{
    if (message->command == FVM_SORDER_OFFER)
    {
        FVM_SOrderControl reply = *message; /* Correlated reply, no implicit TX authority. */
        check(message->lane == 71 && message->limit == 128);
        if (ORDERED_CASE == 3)
        {
            data(7, 0, "premature");
            return;
        }
        reply.command = ORDERED_CASE == 1 && message->operation.low == 1 ? FVM_SORDER_REJECT : FVM_SORDER_ACCEPT;
        if (reply.command == FVM_SORDER_REJECT)
        {
            service(&reply);
            return;
        }
        reply.limit = 96;
        reply.window = number(4);
        if (ORDERED_CASE == 2)
        {
            reply.lane++;
            service(&reply);
            return;
        }
        service(&reply);
        service(&reply); /* Completed offer duplicate remains idempotent. */
        append(0, "native", 6);
        reply.command = FVM_SORDER_OFFER;
        reply.operation = number(1);
        reply.lane = 91;
        reply.limit = 128;
        reply.window = number(8);
        service(&reply);
        service(&reply);
    }
    else if (message->command == FVM_SORDER_ACCEPT)
    {
        check(message->lane == 91 && message->limit == 128 && message->window.low == 8);
        if (!remote_reset.command)
        {
            incoming();
        }
    }
    else if (message->command == FVM_SORDER_RENEW)
    {
        check(message->tag == 7 && message->lane == 71 && message->operation.low == 1 && host_records == (ORDERED_CASE == 5 ? 2U : 1U));
        check(message->boundary.low == (ORDERED_CASE == 5 ? 0xffffffffUL : 0));
        host_reset = *message; /* Intentionally hold ACK until another client's message arrives. */
    }
    else
    {
        check(message->command == FVM_SORDER_ACK && message->tag == 7 && message->lane == 91 &&
            message->operation.low == remote_reset.operation.low && message->boundary.low == remote_reset.boundary.low);
        data(7, 0, "new");
#ifdef ORDERED_FILE_TEST
        data(7, 1, ""); /* Empty application payload remains a nonempty S-record, not EOF. */
#endif
    }
}


/* EN/RU: Verify exact host packet ordinal and local-zero isolation; unrelated data releases held ACK.
 * Проверить номер host-пакета и изоляцию локального нуля; чужие данные освобождают удержанное ACK.
 */
static void packet(const FVM_SRecord *outer)
{
    FVM_SOrderedRecord view; /* Borrowed verified inner packet. */
    size_t used; /* Exact packet extent. */
    check(outer->tag == 71 && fvm_sordered_read(outer->payload, outer->size, 96, &view, &used) == FVM_S_OK && used == outer->size);
    uvlong ordinal = ((uvlong)view.ordinal.high << 32) | view.ordinal.low; /* Exact full number. */
    if (view.record.tag == 7)
    {
        uvlong expected = ORDERED_CASE == 5 && host_records < 2 ? ~0ULL - 1 + host_records : 0; /* Agreed fixture checkpoint. */
        check(ordinal == expected);
        host_records++;
#ifdef ORDERED_GUEST_FILE_TEST
        /* EN/RU: Acknowledge the exact init marker before application output.
         * Подтвердить точный маркер init до вывода приложения. */
        check(host_records == 1 && view.record.size == 4 && !memcmp(view.record.payload, "out0", 4));
        data(9, 2, "started");
#endif
        if (host_records == (ORDERED_CASE == 5 ? 3U : 2U))
        {
            check(host_zero && view.record.size == 6 && !memcmp(view.record.payload, "outnew", 6));
            data(9, 3, "verified");
        }
    }
    else if (view.record.tag == 9)
    {
        check(!ordinal && host_reset.command && view.record.size == 9 && !memcmp(view.record.payload, "unrelated", 9));
        FVM_SOrderControl ack = host_reset; /* Release only this sender's selected tag. */
        ack.command = FVM_SORDER_ACK;
        service(&ack);
        service(&ack);
        data(9, 2, "tx-ready");
    }
    else
    {
        check(!view.record.tag && !ordinal && view.record.size == 9 && !memcmp(view.record.payload, "local-out", 9));
        host_zero = 1;
    }
#ifdef ORDERED_GUEST_FILE_TEST
    if (host_records == 1 && host_zero)
    {
        data(9, 3, "guest-verified"); /* Native guest fixture receipt, not a new production protocol. */
    }
#endif

}


/* EN/RU: Consume original retained read on reply, flush or disconnect, server owner only.
 * Освободить исходный read при reply, flush или disconnect; только server owner.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Transfer owned handle before free. */
    pending = nil;
    styxreadfree(read);
}


/* EN/RU: Original three-byte reads, including deferred idle reads; no empty-as-EOF shortcut.
 * Исходные трёхбайтные чтения, включая отложенные пустые; ожидание не подменяется EOF.
 */
char *terminalread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    if (offset < 0 || (uvlong)offset > extent)
    {
        return Ebadarg;
    }
    if ((uvlong)offset == extent)
    {
        check(!pending);
        pending = styxdeferread(server, nil, release);
        *count = 0;
        return pending ? nil : Enomem;
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


/* EN/RU: Acknowledge exactly two bytes per Twrite, decode retained original frames, publish deferred output.
 * Подтверждать ровно до двух байт Twrite, собирать исходные frames и выдавать отложенный output.
 */
char *terminalwrite(char *bytes, ulong *count, vlong offset)
{
    check(offset >= 0 && (uvlong)offset == written);
    if (!initialized)
    {
        check(fvm_sstream_init(&decoder, storage, sizeof(storage), 512) == FVM_S_OK);
        initialized = 1;
    }
    if (*count > 2)
    {
        *count = 2;
    }
    written += *count;
    for (size_t at = 0; at < *count;)
    {
        FVM_SRecord view; /* Borrowed original outer frame. */
        size_t used; /* Exact consumed prefix. */
        FVM_SResult result = fvm_sstream_push(&decoder, bytes + at, *count - at, &view, &used); /* Incremental result. */
        check(used && (result == FVM_S_OK || result == FVM_S_MORE));
        at += used;
        if (result == FVM_S_OK)
        {
            if (!view.tag)
            {
                FVM_SOrderControl message; /* Checked named command. */
                check(fvm_sorder_control_read(view.payload, view.size, &message) == FVM_S_OK);
                control(&message);
            }
            else
            {
                packet(&view);
            }
            check(fvm_sstream_reset(&decoder) == FVM_S_OK);
        }
    }
    if (pending && (uvlong)styxreadoffset(pending) < extent)
    {
        vlong at = styxreadoffset(pending); /* Original pending offset. */
        ulong count = styxreadlimit(pending); /* Original pending capacity. */
        if (count > extent - (size_t)at)
        {
            count = extent - (size_t)at;
        }
        if (count > 3)
        {
            count = 3;
        }
        check(styxreadreply(pending, output + at, count, nil));
    }
    return nil;
}
