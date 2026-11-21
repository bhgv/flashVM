/* Copyright (c) 2026 bhgv
 * EN: Exact Unibus scalar wire format checks and malformed record rejection.
 * RU: Проверка формата скаляров Unibus и отклонения неверных записей.
 */
#include "scalar_record.h"
#include "scalar_route.h"
#include "scalar_verify.h"
#include "tuple_record.h"
#include "call_record.h"
#include "ref_pair_record.h"
#include "load_profile.h"
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
        styxunibusmailboxseal(provider);
        ok = ok && styxscalarcallfile(provider, 17, 99) != DVM_LAUNCH_DELIVERED;
        got = dvm_vfs_read_chunk(&reader, bytes, sizeof(bytes));
        ok = ok && got.status == FVM_OK &&
            styxscalarread(17, bytes, got.count, &kind, &value) && kind == STYX_SCALAR_CALL && value == 88 &&
            dvm_vfs_read_chunk(&reader, bytes, sizeof(bytes)).stop == DVM_IO_EOF;
    }
    dvm_vfs_close(&reader);
    dvm_provider_release(provider);
    return ok;
}


/* EN: A valid return followed by a missing update must leave both outputs intact.
 * RU: При корректном результате без обновления оба выходных значения сохраняются. */
static int ref_reply(void)
{
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY], payload[STYX_FUNCTION_FRAME_CAPACITY];
    const StyxRefReply expected = {{INT32_MIN, INT64_MIN}, {INT32_MAX, INT64_MAX}};
    StyxRefReply decoded = {0};
    size_t size, first;
    if (!styxrefreplywrite(&expected, bytes, sizeof(bytes), &size) ||
        !styxrefreplyread(bytes, size, &decoded) ||
        decoded.result.narrow != expected.result.narrow || decoded.result.wide != expected.result.wide ||
        decoded.updated.narrow != expected.updated.narrow || decoded.updated.wide != expected.updated.wide ||
        !styxtuplewrite(STYX_TUPLE_REPLY_TAG, 0, &expected.updated, payload, sizeof(payload), &first) ||
        fvm_srecord_write(STYX_REF_REPLY_TAG, payload, first, bytes, sizeof(bytes), &size) != FVM_S_OK)
    {
        return 0;
    }
    return !styxrefreplyread(bytes, size, &decoded) &&
        decoded.result.narrow == expected.result.narrow && decoded.result.wide == expected.result.wide &&
        decoded.updated.narrow == expected.updated.narrow && decoded.updated.wide == expected.updated.wide;
}


/* EN: Verify zero and one positions, nested shape, and unchanged output on bad input.
 * RU: Проверить ноль и одну позицию, вложенность и сохранение вывода при ошибке. */
static int argument_list(void)
{
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size;
    const StyxCallValues empty = {0}, one = {1, {INT32_MIN}};
    const StyxCallValues many = {4, {INT32_MIN, INT32_MAX, INT64_MIN, INT64_MAX}};
    StyxCallValues decoded;
    if (!styxcallrecordwrite(STYX_CALL_LIST_REQUEST_TAG, "()", &empty, bytes, sizeof(bytes), &size) ||
        !styxcallrecordread(STYX_CALL_LIST_REQUEST_TAG, "()", bytes, size, &decoded) || decoded.count ||
        styxcallrecordwrite(STYX_CALL_LIST_REQUEST_TAG, "((((()))))", &empty, bytes, sizeof(bytes), &size) ||
        !styxcallrecordwrite(STYX_CALL_LIST_REQUEST_TAG, "(i)", &one, bytes, sizeof(bytes), &size) ||
        !styxcallrecordread(STYX_CALL_LIST_REQUEST_TAG, "(i)", bytes, size, &decoded) ||
        decoded.count != 1 || decoded.cells[0] != INT32_MIN ||
        !styxcallrecordwrite(STYX_CALL_LIST_REQUEST_TAG, styxcall_list_args, &many, bytes, sizeof(bytes), &size) ||
        !styxcallrecordread(STYX_CALL_LIST_REQUEST_TAG, styxcall_list_args, bytes, size, &decoded) ||
        decoded.count != many.count || memcmp(many.cells, decoded.cells, sizeof(many.cells)))
    {
        return 0;
    }
    return !styxcallrecordread(STYX_CALL_LIST_REQUEST_TAG, "(iibb)", bytes, size, &decoded) &&
        !styxcallrecordread(STYX_CALL_LIST_REQUEST_TAG, "((i(ib)b))", bytes, size, &decoded) &&
        !styxcallrecordread(STYX_CALL_LIST_REQUEST_TAG, styxcall_list_args, bytes, size - 1, &decoded) &&
        decoded.count == many.count && !memcmp(many.cells, decoded.cells, sizeof(many.cells)) &&
        !styxcallrecordwrite(STYX_CALL_LIST_REQUEST_TAG, "(iibi)", &many, bytes, sizeof(bytes), &size);
}


/* EN: Test both maps and exact rejection of an impossible argument ID.
 * RU: Проверить обе таблицы и отказ при невозможном номере аргумента. */
static int ref_pair(void)
{
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size;
    StyxRefPair value = {.count = 1, .objects = {{INT32_MIN, INT64_MIN}, {INT32_MAX, INT64_MAX}},
        .result = {INT32_MAX, INT64_MIN}}, decoded;
    if (!styxrefpairwrite(STYX_REF_PAIR_REQUEST_TAG, &value, bytes, sizeof(bytes), &size) ||
        !styxrefpairread(STYX_REF_PAIR_REQUEST_TAG, bytes, size, &decoded) || decoded.count != 1 ||
        decoded.objects[0].narrow != INT32_MIN || decoded.objects[0].wide != INT64_MIN || decoded.objects[1].wide)
    {
        return 0;
    }
    value.count = 2;
    if (!styxrefpairwrite(STYX_REF_PAIR_REPLY_TAG, &value, bytes, sizeof(bytes), &size) ||
        !styxrefpairread(STYX_REF_PAIR_REPLY_TAG, bytes, size, &decoded) || decoded.count != 2 ||
        decoded.objects[1].narrow != INT32_MAX || decoded.objects[1].wide != INT64_MAX ||
        decoded.result.narrow != INT32_MAX || decoded.result.wide != INT64_MIN ||
        styxrefpairread(STYX_REF_PAIR_REPLY_TAG, bytes, size - 1, &decoded))
    {
        return 0;
    }
    unsigned char bad[STYX_FUNCTION_FRAME_CAPACITY];
    unsigned char invalid[STYX_FUNCTION_FRAME_CAPACITY];
    const unsigned char impossible[] = {0, 2};
    FVM_SRecord outer, map;
    size_t used, map_size;
    return styxrefpairwrite(STYX_REF_PAIR_REQUEST_TAG, &value, bytes, sizeof(bytes), &size) &&
        fvm_srecord_read(bytes, size, sizeof(bytes), &outer, &used) == FVM_S_OK &&
        fvm_srecord_read(outer.payload, outer.size, sizeof(bytes), &map, &map_size) == FVM_S_OK &&
        fvm_srecord_write(1, impossible, sizeof(impossible), bad, sizeof(bad), &used) == FVM_S_OK &&
        fvm_srecord_write_pair(STYX_REF_PAIR_REQUEST_TAG, bad, used, outer.payload + map_size,
            outer.size - map_size, invalid, sizeof(invalid), &size) == FVM_S_OK &&
        !styxrefpairread(STYX_REF_PAIR_REQUEST_TAG, invalid, size, &decoded) &&
        decoded.count == 2 && decoded.objects[1].wide == INT64_MAX && decoded.result.wide == INT64_MIN;
}


/* EN: Round-trip every admitted load signature and reject incomplete or foreign offers.
 * RU: Проверить все допустимые сигнатуры load и отклонение неполных или чужих предложений. */
static int loaded_profile(void)
{
    const unsigned long tags[] = {STYX_FUNCTION_REQUEST_TAG, STYX_FUNCTION_BIG_TAG,
        STYX_TUPLE_REQUEST_TAG, STYX_REF_REQUEST_TAG, STYX_CALL_LIST_REQUEST_TAG, STYX_REF_PAIR_REQUEST_TAG};
    unsigned char bytes[STYX_SCALAR_INTERFACE_CAPACITY];
    size_t used;
    for (size_t i = 0; i < sizeof(tags) / sizeof(tags[0]); i++)
    {
        uint32_t signature = styxscalar_request_signature(tags[i]);
        if (!styxloadedoffer("blend", signature, bytes, sizeof(bytes), &used) ||
            styxloadedaccept("blend", bytes, used) != tags[i] ||
            !styxscalaraccept(STYX_FUNCTION_PROFILE_TAG, bytes, used, "blend", signature) ||
            styxloadedaccept("other", bytes, used) || styxloadedaccept("blend", bytes, used - 1))
        {
            return 0;
        }
    }
    return !styxloadedoffer("blend", 0, bytes, sizeof(bytes), &used);
}


/* EN: Bind a wide shaped profile and reject a contradictory schema with valid framing.
 * RU: Связать расширенный профиль и отказать противоречивой схеме в корректном кадре. */
static int shaped_profile(void)
{
    const char name[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const StyxCallShape shape = {"(i(ib)b)", "((i(ib)))"};
    StyxCallShape decoded;
    unsigned char bytes[STYX_SHAPED_OFFER_MAX], changed[STYX_SHAPED_OFFER_MAX];
    size_t size, used;
    FVM_SRecord record;
    int caller = 1;
    if (!styxshapedoffer(name, DVM_CALL_LIST_SIGNATURE, &shape, bytes, sizeof(bytes), &size) ||
        size <= STYX_SCALAR_INTERFACE_CAPACITY || styxloadedaccept(name, bytes, size) != STYX_CALL_LIST_REQUEST_TAG ||
        !styxscalar_profile_match_shape(bytes, size, name, DVM_CALL_LIST_SIGNATURE, &caller, &decoded) ||
        caller || strcmp(decoded.arguments, shape.arguments) || strcmp(decoded.result, shape.result) ||
        fvm_srecord_read(bytes, size, sizeof(bytes), &record, &used) != FVM_S_OK)
    {
        return 0;
    }
    unsigned char payload[STYX_SHAPED_OFFER_MAX];
    memcpy(payload, record.payload, record.size);
    payload[INTERFACE_HEADER + 3 + strlen(name) + 1] = 'b';
    return fvm_srecord_write(STYX_SHAPED_OFFER_TAG, payload, record.size, changed, sizeof(changed), &used) == FVM_S_OK &&
        !styxloadedaccept(name, changed, used) &&
        !styxscalar_profile_match_shape(changed, used, name, DVM_CALL_LIST_SIGNATURE, &caller, &decoded) &&
        !strcmp(decoded.arguments, shape.arguments) && !strcmp(decoded.result, shape.result);
}


/* EN: Admit bounded scalar layouts and reject pointers or a fifth cell.
 * RU: Допустить ограниченные скалярные раскладки и отклонить указатели или пятую ячейку. */
static int dynamic_profile(void)
{
    const StyxCallShape good = {"((bi)ib)", "((b(ii)b))"};
    const StyxCallShape empty = {"()", "()"};
    const StyxCallShape too_wide = {"(iiiii)", "(b)"};
    const StyxCallShape reference = {"(r[ib])", "(b)"};
    unsigned char bytes[STYX_SHAPED_OFFER_MAX];
    size_t a, r, used;
    return styxshapecells(&good, &a, &r) && a == 4 && r == 4 &&
        styxshapecells(&empty, &a, &r) && !a && !r &&
        !styxshapecells(&too_wide, &a, &r) && !styxshapecells(&reference, &a, &r) &&
        styxshapedoffer("sample", 17, &good, bytes, sizeof(bytes), &used) &&
        styxloadedaccept("sample", bytes, used) == STYX_DYNAMIC_REQUEST_TAG;
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
    if (!dynamic_profile() || !shaped_profile())
    {
        fputs("compiled type offer failed\n", stderr);
        return 1;
    }
    if (!loaded_profile())
    {
        fputs("loaded export offer failed\n", stderr);
        return 1;
    }
    if (!argument_list())
    {
        fputs("ordered argument list failed\n", stderr);
        return 1;
    }
    if (!ref_pair())
    {
        fputs("ref pair identity or framing failed\n", stderr);
        return 1;
    }
    if (!ref_reply())
    {
        fputs("ref reply round trip or staging failed\n", stderr);
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
