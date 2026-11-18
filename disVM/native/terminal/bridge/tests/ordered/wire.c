/* Copyright (c) 2026 bhgv
 * EN: Original Styx terminal peer for real local value channels and negotiated order envelopes.
 * RU: Исходный Styx peer для настоящих локальных value-каналов и согласованных конвертов порядка.
 */
#include <stdio.h>
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sordctl.h"
#include "disvm/ui_frame.h"
static unsigned char output[16384]; /* Sole server-owner output log. */
static size_t extent; /* Published exact prefix. */
static unsigned char storage[4096]; /* Original incoming stream storage. */
static FVM_SStream decoder; /* Incoming ordinary S-record decoder. */
static size_t written; /* Confirmed Twrite prefix. */
static int initialized; /* Original decoder ready. */
static int published; /* Initial frames emitted once despite duplicate acceptance. */
static StyxRead *pending; /* Original retained idle read. */


/* EN/RU: Fatal explicit native wire contract; no optimized-away assertion.
 * Явная обязательная проверка протокола; без удаляемого оптимизатором assertion.
 */
static void check(int condition)
{
    if (!condition)
    {
        fprintf(stderr, "ordered terminal wire case %d\n", ORDERED_TERMINAL_CASE);
        exit(3);
    }
}


/* EN/RU: Append one complete outer S-record to server-owned output.
 * Добавить одну полную внешнюю S-запись в собственный output сервера.
 */
static void append(ulong tag, const void *bytes, size_t size)
{
    size_t used; /* Full encoded frame extent. */
    check(fvm_srecord_write(tag, bytes, size, output + extent, sizeof(output) - extent, &used) == FVM_S_OK);
    extent += used;
}


/* EN/RU: Encode one named control body into ordinary outer root0.
 * Кодировать тело именованной службы в обычный внешний root0.
 */
static void service(const FVM_SOrderControl *message)
{
    unsigned char bytes[FVM_SORDER_CTL_MAX]; /* Private complete named body. */
    size_t used; /* Encoded body extent. */
    check(fvm_sorder_control_write(message, bytes, sizeof(bytes), &used) == FVM_S_OK);
    append(0, bytes, used);
}


/* EN/RU: Append checked prefix+inner record on incoming lane91; fixture ordinals fit the low limb.
 * Добавить префикс и внутреннюю запись на входящей линии91; номера fixture помещаются в low.
 */
static void data(ulong tag, ulong ordinal, const void *bytes, size_t size)
{
    unsigned char encoded[2048]; /* Complete original inner packet. */
    FVM_SOrdinal number = {0, ordinal}; /* Explicit portable sequence. */
    size_t used; /* Combined encoded extent. */
    check(fvm_sordered_write(&number, tag, bytes, size, encoded, sizeof(encoded), &used) == FVM_S_OK);
    append(91, encoded, used);
}


/* EN/RU: Native fixture UI tag0 is explicitly application-local; all other cases use inner7.
 * В fixture UI-тег0 явно клиентский; остальные варианты используют внутренний7.
 */
static ulong ui_tag(void)
{
    return ORDERED_TERMINAL_CASE == 1 ? 0 : 7;
}


/* EN/RU: Emit one or a full queue of fvd1 values, then independent tag9 receipts and a native root notice.
 * Выдать один fvd1 либо полную очередь, затем независимые квитанции тега9 и native root-уведомление.
 */
static void incoming(void)
{
    if (published)
    {
        return;
    }
    published = 1;
    if (ORDERED_TERMINAL_CASE == 4)
    {
        data(ui_tag(), 0, "invalid-fvd1", 12);
        return;
    }
    unsigned count = ORDERED_TERMINAL_CASE == 2 ? DVM_UI_PENDING_MAX + 2 : 1; /* Real local RX pressure. */
    for (unsigned i = 0; i < count; i++)
    {
        DVM_UIMessage message = {0}; /* Pure copied values, no VM pointers. */
        char bytes[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX]; /* Canonical counted fvd1. */
        size_t used; /* Encoded frame extent. */
        message.kind = DVM_UI_REQUEST;
        message.id = 42 + i;
        message.code = 7;
        message.first = -2147483647L - 1;
        message.size = 4;
        memcpy(message.text, "A\0\320\257", 4);
        check(dvm_ui_frame_encode(&message, bytes, sizeof(bytes), &used) == FVM_OK);
        data(ui_tag(), i, bytes, used);
    }
    data(9, 0, "independent", 11);
    append(0, "native", 6);
}


/* EN/RU: Independently validate native offers and copied UI output; root control is never parsed from an inner0.
 * Независимо проверить предложения и копию UI; внутренний0 не разбирается как root-служба.
 */
static void command(const FVM_SRecord *record)
{
    if (!record->tag)
    {
        FVM_SOrderControl message; /* Checked named control. */
        check(fvm_sorder_control_read(record->payload, record->size, &message) == FVM_S_OK);
        if (message.command == FVM_SORDER_OFFER)
        {
            check(message.lane == 71 && message.limit == 2048);
            message.command = FVM_SORDER_ACCEPT;
            if (ORDERED_TERMINAL_CASE == 3)
            {
                message.limit = 256; /* Legitimate smaller remote TX profile. */
            }
            service(&message);
            message.command = FVM_SORDER_OFFER;
            message.operation.high = 0;
            message.operation.low = 1;
            message.lane = 91;
            message.limit = 2048;
            message.window.high = 0;
            message.window.low = 8;
            service(&message);
        }
        else
        {
            check(message.command == FVM_SORDER_ACCEPT && message.lane == 91);
            incoming();
        }
        return;
    }
    FVM_SOrderedRecord view; /* Checked original packet, prefix remains outside inner S-record. */
    size_t used; /* Exact packet extent. */
    DVM_UIMessage message; /* Copied original frame values. */
    check(record->tag == 71 && fvm_sordered_read(record->payload, record->size, 2048, &view, &used) == FVM_S_OK && used == record->size);
    check(view.record.tag == ui_tag() && !view.ordinal.high && !view.ordinal.low);
    check(dvm_ui_frame_decode(view.record.payload, view.record.size, &message) == FVM_OK &&
        message.kind == DVM_UI_REPLY && message.id == 43 && message.size == 5 && !memcmp(message.text, "reply", 5));
    data(9, 1, "done", 4);
}


/* EN/RU: Consume original deferred read after reply/flush/disconnect, server owner only.
 * Поглотить исходный отложенный read после reply/flush/disconnect; только server owner.
 */
static void release(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Transfer ownership before freeing. */
    pending = nil;
    styxreadfree(read);
}


/* EN/RU: Original three-byte reads; empty live source defers rather than masquerading as EOF.
 * Исходные трёхбайтные чтения; пустой живой источник откладывается, а не подменяется EOF.
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


/* EN/RU: Exactly two acknowledged bytes per original Twrite; retain complete framing across calls.
 * Ровно до двух подтверждённых байт на исходный Twrite; сохранять обрамление между вызовами.
 */
char *terminalwrite(char *bytes, ulong *count, vlong offset)
{
    check(offset >= 0 && (uvlong)offset == written);
    if (!initialized)
    {
        check(fvm_sstream_init(&decoder, storage, sizeof(storage), 3072) == FVM_S_OK);
        initialized = 1;
    }
    if (*count > 2)
    {
        *count = 2;
    }
    written += *count;
    for (size_t at = 0; at < *count;)
    {
        FVM_SRecord record; /* Borrowed ordinary original S-record. */
        size_t used; /* Exact consumed fragment. */
        FVM_SResult status = fvm_sstream_push(&decoder, bytes + at, *count - at, &record, &used); /* Checked stream status. */
        check(used && (status == FVM_S_OK || status == FVM_S_MORE));
        at += used;
        if (status == FVM_S_OK)
        {
            command(&record);
            check(fvm_sstream_reset(&decoder) == FVM_S_OK);
        }
    }
    if (pending && (uvlong)styxreadoffset(pending) < extent)
    {
        vlong at = styxreadoffset(pending); /* Original pending offset. */
        ulong size = styxreadlimit(pending); /* Original capacity. */
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
