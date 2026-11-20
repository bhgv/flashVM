/* Copyright (c) 2026 bhgv
 * EN: Exact Unibus scalar wire format checks and malformed record rejection.
 * RU: Проверка формата скаляров Unibus и отклонения неверных записей.
 */
#include "scalar_record.h"
#include "scalar_route.h"
#include "unibus_inbox.h"
#include "flashvm/srecord.h"
#include <stdio.h>
#include <string.h>


/* EN/RU: Return the argument or a fixture error, recording each admitted call.
 * Вернуть аргумент или тестовую ошибку, считая допущенные вызовы. */
static int method(void *state, int64_t argument, int64_t *result)
{
    ++*(int *)state;
    *result = argument < 0 ? 42 : argument;
    return argument >= 0;
}


/* EN/RU: Check single-slot backpressure and unchanged record after a short read.
 * Проверить отказ записи в занятый слот и сохранение записи после короткого чтения. */
static int mailbox(void)
{
    DVM_Provider *provider = styxunibusmailbox(17);
    DVM_VfsHandle reader = {0};
    int ok = provider && dvm_provider_open(provider, ".", "r", &reader) == FVM_OK;
    if (ok)
    {
        unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
        ok = styxscalarcallfile(provider, 17, 77) == DVM_LAUNCH_DELIVERED &&
            styxscalarcallfile(provider, 17, 88) != DVM_LAUNCH_DELIVERED &&
            dvm_vfs_read_chunk(&reader, bytes, 1).count == 0;
        DVM_IoResult got = dvm_vfs_read_chunk(&reader, bytes, sizeof(bytes));
        StyxScalarKind kind;
        int64_t value;
        ok = ok && got.status == FVM_OK && (got.flags & DVM_IO_RECORD) &&
            styxscalarread(17, bytes, got.count, &kind, &value) && kind == STYX_SCALAR_CALL && value == 77 &&
            dvm_vfs_read_chunk(&reader, bytes, sizeof(bytes)).stop == DVM_IO_AGAIN &&
            styxscalarcallfile(provider, 17, 88) == DVM_LAUNCH_DELIVERED;
        got = dvm_vfs_read_chunk(&reader, bytes, sizeof(bytes));
        ok = ok && got.status == FVM_OK &&
            styxscalarread(17, bytes, got.count, &kind, &value) && kind == STYX_SCALAR_CALL && value == 88;
    }
    dvm_vfs_close(&reader);
    dvm_provider_release(provider);
    return ok;
}


/* EN/RU: Check full signed width, tags, integrity and domain separation.
 * Проверить всю знаковую ширину, теги, целостность и виды записи. */
int main(void)
{
    unsigned char bytes[64];
    const int64_t values[] = {INT64_MIN, -257, -1, 0, 256, INT64_MAX};
    for (int kind = STYX_SCALAR_CALL; kind <= STYX_SCALAR_ERROR; kind++)
    {
        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
        {
            size_t size;
            StyxScalarKind decoded = 0;
            int64_t value = 0;
            if (!styxscalarwrite(17, kind, values[i], bytes, sizeof(bytes), &size) ||
                !styxscalarread(17, bytes, size, &decoded, &value) ||
                decoded != (StyxScalarKind)kind || value != values[i] ||
                styxscalarread(18, bytes, size, &decoded, &value) ||
                styxscalarread(17, bytes, size - 1, &decoded, &value))
            {
                fputs("scalar record round trip failed\n", stderr);
                return 1;
            }
            bytes[size] = 0;
            if (styxscalarread(17, bytes, size + 1, &decoded, &value) ||
                decoded != (StyxScalarKind)kind || value != values[i])
            {
                fputs("scalar refusal modified output\n", stderr);
                return 1;
            }
        }
    }
    size_t size;
    StyxScalarKind decoded;
    int64_t value;
    const unsigned char expected[] = {1, 0x80, 1, 2, 3, 4, 5, 6, 7};
    FVM_SRecord record;
    size_t consumed;
    if (!styxscalarwrite(17, STYX_SCALAR_CALL, INT64_MIN + INT64_C(0x01020304050607),
            bytes, sizeof(bytes), &size) ||
        fvm_srecord_read(bytes, size, 32, &record, &consumed) != FVM_S_OK ||
        consumed != size || record.size != sizeof(expected) ||
        memcmp(record.payload, expected, sizeof(expected)) ||
        styxscalarwrite(0, STYX_SCALAR_CALL, 0, bytes, sizeof(bytes), &size) ||
        styxscalarwrite(17, 0, 0, bytes, sizeof(bytes), &size) ||
        styxscalarwrite(17, STYX_SCALAR_CALL, 0, bytes, 1, &size))
    {
        fputs("scalar record wire check failed\n", stderr);
        return 1;
    }
    unsigned char bad[9] = {4};
    if (fvm_srecord_write(17, bad, sizeof(bad), bytes, sizeof(bytes), &size) != FVM_S_OK ||
        styxscalarread(17, bytes, size, &decoded, &value))
    {
        fputs("unknown scalar record kind accepted\n", stderr);
        return 1;
    }
    unsigned char reply[STYX_SCALAR_RECORD_CAPACITY];
    size_t reply_size;
    int calls = 0;
    if (!styxscalarwrite(17, STYX_SCALAR_CALL, -7, bytes, sizeof(bytes), &size) ||
        styxscalarrespond(17, bytes, size, 29, method, &calls, reply, 1, &reply_size) ||
        styxscalarrespond(18, bytes, size, 29, method, &calls, reply, sizeof(reply), &reply_size) ||
        calls ||
        !styxscalarrespond(17, bytes, size, 29, method, &calls, reply, sizeof(reply), &reply_size) ||
        !styxscalarread(29, reply, reply_size, &decoded, &value) ||
        decoded != STYX_SCALAR_ERROR || value != 42 || calls != 1 ||
        !styxscalarwrite(17, STYX_SCALAR_CALL, 99, bytes, sizeof(bytes), &size) ||
        !styxscalarrespond(17, bytes, size, 29, method, &calls, reply, sizeof(reply), &reply_size) ||
        !styxscalarread(29, reply, reply_size, &decoded, &value) ||
        decoded != STYX_SCALAR_VALUE || value != 99 || calls != 2)
    {
        fputs("scalar method dispatch failed\n", stderr);
        return 1;
    }
    if (!mailbox())
    {
        fputs("reusable Unibus slot failed\n", stderr);
        return 1;
    }
    puts("PASS exact signed scalar Unibus records and method dispatch");
    return 0;
}
