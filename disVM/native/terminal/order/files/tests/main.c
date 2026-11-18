/* Copyright (c) 2026 bhgv
 * EN: Real original-Styx record files, filtered grants, partial reads, exact writes and independent cancellation.
 * RU: Настоящие record-файлы через исходный Styx, фильтрация grants, short reads, точные writes и независимая отмена.
 */
#include "../files.h"
#include "broker/attach.h"
#include "mount.h"
#include "cancel.h"
#include "flashvm/srecord.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Original fixture connection, owned until shutdown/free; physical owner survives until fixturefree.
 * Исходное fixture-соединение, owned до shutdown/free, physical owner живёт до fixturefree.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Join original fixture server and pumps after protocol users retire.
 * Завершить исходный fixture server и pumps после protocol users.
 */
extern void linkfixturefree(void);


/* EN/RU: Exercise sticky/active RX and selected-counter TX cancellation without stopping shared peer.
 * Проверить sticky/active отмену RX и TX выбранного счётчика без stop общего peer.
 */
extern void filewaitcancel(StyxOrdered *peer, unsigned tag, int writing);


/* EN/RU: Force retirement publication before engine close; source remains uncancelled, peer borrowed through join.
 * Принудительно опубликовать retirement до engine close; source не отменяется, peer borrowed до join.
 */
extern void filewaitstop(StyxOrdered *peer);


/* EN/RU: Fatal mandatory predicate with line diagnostic, never optimized away.
 * Обязательная fatal-проверка с номером строки, не удаляется оптимизатором.
 */
static void check(int valid, int line)
{
    if (!valid)
    {
        fprintf(stderr, "ordered record file line %d\n", line);
        exit(2);
    }
}
#define CHECK(value) check((value) != 0, __LINE__) /* Always evaluated fixture condition. */


/* EN/RU: Read one original S-record, optionally in one-byte fragments; caller retains exclusive FD through return.
 * Прочитать одну исходную S-запись, опционально по одному байту; caller удерживает exclusive FD до возврата.
 */
static void take(DVM_VfsHandle *handle, unsigned long tag, const char *text, int fragmented)
{
    unsigned char encoded[256]; /* Exactly one original application frame, never outer root traffic. */
    size_t extent = 0; /* Accumulated short-read prefix. */
    DVM_IoResult result; /* Exact per-call acknowledgement. */
    do
    {
        result = dvm_vfs_read_chunk(handle, encoded + extent, fragmented ? 1 : sizeof(encoded) - extent);
        CHECK(result.status == FVM_OK && result.count && extent + result.count <= sizeof(encoded));
        extent += result.count;
    } while (!(result.flags & DVM_IO_RECORD));
    FVM_SRecord record; /* Borrowed independent original codec view. */
    size_t used; /* Full original record extent. */
    CHECK(fvm_srecord_read(encoded, extent, 256, &record, &used) == FVM_S_OK && used == extent &&
        record.tag == tag && record.size == strlen(text) && !memcmp(record.payload, text, record.size));
}


/* EN/RU: Write exactly one canonical application record, no transport or privileged service fields.
 * Записать ровно одну каноническую клиентскую запись без transport либо privileged service-полей.
 */
static void send(DVM_VfsHandle *handle, unsigned long tag, const char *text)
{
    unsigned char encoded[256]; /* Complete ordinary client frame. */
    size_t size; /* Exact encoded extent. */
    CHECK(fvm_srecord_write(tag, text, strlen(text), encoded, sizeof(encoded), &size) == FVM_S_OK);
    DVM_IoResult result = dvm_vfs_write_chunk(handle, encoded, size); /* One immutable admission. */
    CHECK(result.status == FVM_OK && result.count == size && (result.flags & DVM_IO_RECORD));
}


/* EN/RU: Actual record-file admission and lifetime; no guest/display or fake transport responses.
 * Настоящий допуск и lifetime record-file, без guest/display и фиктивных ответов транспорта.
 */
int main(void)
{
    StyxClient *client = linkfixture(); /* Own original connection independently of broker files. */
    CHECK(client && styxclientretain(client));
    DVM_Provider *grant = styxmountproviderat(client, ".", FVM_ACCESS_MASK); /* Consumes extra client pin. */
    DVM_TerminalChannels channels = {0}; /* Exact granted stream pair. */
    channels.provider = grant;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    unsigned long tags[] = {0, 7, 9, 99}; /* Last tag is a deliberately idle independent client. */
    StyxOrderedOptions options = {sizeof(options), 128, 16, 71, tags, 4}; /* Explicit ordered profile. */
    StyxOrdered *peer = styxbidirorderedchannels(client, &channels, &options); /* Transferred to native file owner below. */
    CHECK(peer && styxorderedwait(peer, STYX_ORDER_TX, 3000) && styxorderedwait(peer, STYX_ORDER_RX, 3000));
    StyxOrderedFiles *files = styxorderedfiles(peer); /* Own broker lifecycle, borrow peer while files remains alive. */
    CHECK(files && !styxorderedfile(files, 8, FVM_ACCESS_READ) && !styxorderedfile(files, 7, FVM_ACCESS_EXEC));
    DVM_Provider *seven = styxorderedfile(files, 7, FVM_ACCESS_READ | FVM_ACCESS_WRITE); /* Independent filtered factories. */
    DVM_Provider *nine = styxorderedfile(files, 9, FVM_ACCESS_READ | FVM_ACCESS_WRITE); /* No ability to choose another client. */
    DVM_Provider *zero = styxorderedfile(files, 0, FVM_ACCESS_READ | FVM_ACCESS_WRITE); /* Application-local0 stays nested. */
    DVM_Provider *readonly = styxorderedfile(files, 7, FVM_ACCESS_READ); /* Ceiling cannot be bypassed through open mode. */
    CHECK(seven && nine && zero && readonly && !styxorderedfilesfree(files));
    DVM_VfsHandle a = {0}; /* Independent FD cursors, pinned through wrapper. */
    DVM_VfsHandle b = {0}; /* Independent tag9 record file. */
    DVM_VfsHandle z = {0}; /* Independent application0 record file. */
    DVM_VfsHandle denied = {0}; /* Must stay empty on denied open. */
    CHECK(dvm_provider_open(readonly, ".", "wo", &denied) != FVM_OK && !denied.ops);
    CHECK(dvm_provider_open(seven, ".", "rw", &a) == FVM_OK && dvm_provider_open(nine, ".", "rw", &b) == FVM_OK &&
        dvm_provider_open(zero, ".", "rw", &z) == FVM_OK);
    dvm_provider_release(seven);
    dvm_provider_release(nine);
    dvm_provider_release(zero);
    dvm_provider_release(readonly);
    CHECK(!styxorderedfilesfree(files)); /* Open wrapper pins survive caller factory release. */
    take(&b, 9, "nine", 0);
    take(&a, 7, "zero", 1);
    DVM_Cancel *cancel = dvm_cancel_create(); /* Operation-local sticky cancellation, leaves ready data/ordinals intact. */
    DVM_CancelScope scope = {0}; /* Exclusive current native FD operation scope. */
    CHECK(cancel && dvm_cancel_enter(&scope, cancel));
    dvm_cancel_request(cancel);
    unsigned char cancelled_bytes[256]; /* Must receive no prefix when cancellation already won. */
    DVM_IoResult cancelled = dvm_vfs_read_chunk(&a, cancelled_bytes, sizeof(cancelled_bytes));
    CHECK(cancelled.stop == DVM_IO_CANCELLED && !cancelled.count);
    CHECK(dvm_cancel_leave(&scope));
    dvm_cancel_release(cancel);
    unsigned char bad[64]; /* Invalid/foreign frames must consume no packet ordinal. */
    size_t size; /* Encoded foreign record. */
    CHECK(fvm_srecord_write(0, "forbidden", 9, bad, sizeof(bad), &size) == FVM_S_OK);
    DVM_IoResult refused = dvm_vfs_write_chunk(&a, bad, size); /* Private root-looking bytes stay unauthorized here. */
    CHECK(refused.status != FVM_OK && !refused.count && !(refused.flags & DVM_IO_UNCERTAIN));
    CHECK(fvm_srecord_write(7, "bad", 3, bad, sizeof(bad), &size) == FVM_S_OK);
    bad[0] ^= 1;
    refused = dvm_vfs_write_chunk(&a, bad, size);
    CHECK(refused.status != FVM_OK && !refused.count);
    send(&a, 7, "out0");
    CHECK(styxorderedrenew(peer, 7));
    filewaitcancel(peer, 7, 1); /* Selected TX reset is intentionally awaiting peer ACK. */
    filewaitcancel(peer, 99, 0); /* Independent RX wait must not close any other client. */
    send(&b, 9, "unrelated");
    send(&z, 0, "local-out");
    take(&b, 9, "rx-live", 0);
    take(&b, 9, "tx-ready", 0);
    send(&a, 7, "outnew");
    take(&b, 9, "verified", 0);
    take(&z, 0, "local-in", 0);
    take(&a, 7, "one", 0);
    take(&a, 7, "two", 0);
    take(&a, 7, "new", 0);
    take(&a, 7, "", 0);
    filewaitstop(peer);
    dvm_vfs_close(&a);
    dvm_vfs_close(&b);
    dvm_vfs_close(&z);
    CHECK(styxorderedfilesfree(files));
    dvm_provider_release(grant);
    CHECK(styxclientalive(client));
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    return 0;
}
