/* Copyright (c) 2026 bhgv
 * EN: Exact Unibus scalar wire format checks and malformed record rejection.
 * RU: Проверка формата скаляров Unibus и отклонения неверных записей.
 */
#include "scalar_record.h"
#include "scalar_route.h"
#include "scalar_verify.h"
#include "tuple_record.h"
#include "call_record.h"
#include "call_scope.h"
#include "ref_pair_record.h"
#include "load_profile.h"
#include "unibus_inbox.h"
#include "flashvm/srecord.h"
#include <stdio.h>
#include <string.h>


#include "managed_records.inc"


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


/* EN: Preserve real bit patterns and reject treating real leaves as integer leaves.
 * RU: Сохранить биты real и отклонить чтение записей real как целых.
 */
static int real_record(void)
{
    /* Binary64 negative zero, quiet NaN with a payload, and positive infinity. */
    const StyxCallShape shape = {"((ff)f)", "(f)"};
    const StyxCallValues input = {.count = 3, .cells = {
        INT64_MIN, INT64_C(0x7ff8000000000042), INT64_C(0x7ff0000000000000)}};
    unsigned char raw[STYX_FUNCTION_FRAME_CAPACITY];
    StyxCallValues output = {0};
    size_t args, result, used;
    return styxshapecells(&shape, &args, &result) && args == 3 && result == 1 &&
        styxcallrecordwrite(STYX_DYNAMIC_REQUEST_TAG, shape.arguments, &input, raw, sizeof(raw), &used) &&
        !styxcallrecordread(STYX_DYNAMIC_REQUEST_TAG, "((bb)b)", raw, used, &output) && !output.count &&
        styxcallrecordread(STYX_DYNAMIC_REQUEST_TAG, shape.arguments, raw, used, &output) &&
        output.count == input.count && !memcmp(output.cells, input.cells, input.count * sizeof(input.cells[0]));
}


/* EN: Exercise adjacent bytes, alignment to a wide field and byte range checks.
 * RU: Проверить соседние байты, выравнивание широкого поля и диапазон byte. */
static int byte_record(void)
{
    const char shape[] = "((yy)b)";
    StyxScalarLayout layout;
    unsigned char frame[16], record[STYX_FUNCTION_FRAME_CAPACITY];
    StyxCallValues values = {.count = 3, .cells = {255, 128, INT64_MIN}}, decoded;
    size_t used;
    if (!styxscalar_layout(shape, &layout) || layout.size != sizeof(frame) || layout.count != 3 ||
        layout.offset[0] != 0 || layout.offset[1] != 1 || layout.offset[2] != 8)
    {
        return 0;
    }
    styxscalar_pack(&layout, &values, frame);
    styxscalar_unpack(&layout, frame, &decoded);
    if (frame[0] != 255 || frame[1] != 128 || frame[2] || decoded.count != values.count ||
        memcmp(decoded.cells, values.cells, values.count * sizeof(values.cells[0])) ||
        !styxcallrecordwrite(STYX_DYNAMIC_REQUEST_TAG, shape, &values, record, sizeof(record), &used) ||
        !styxcallrecordread(STYX_DYNAMIC_REQUEST_TAG, shape, record, used, &decoded) ||
        styxcallrecordread(STYX_DYNAMIC_REQUEST_TAG, "((ii)b)", record, used, &decoded))
    {
        return 0;
    }
    values.cells[1] = 256;
    return !styxcallrecordwrite(STYX_DYNAMIC_REQUEST_TAG, shape, &values, record, sizeof(record), &used);
}


/* EN: Validate nested managed metadata while keeping it outside scalar call admission.
 * RU: Проверить вложенные управляемые типы и отказ скалярного шлюза для них. */
static int managed_profile(void)
{
    const StyxCallShape good = {"(sa[(ys)]r[sa[b]r[i]])", "(a[s])"};
    const StyxCallShape empty_array = {"(a[])", "(s)"};
    const StyxCallShape two_elements = {"(a[si])", "(s)"};
    const StyxCallShape broken_ref = {"(r[a[s])", "(s)"};
    const StyxCallShape too_deep = {"(a[a[a[a[a[a[s]]]]]])", "(s)"};
    return styxshapevalid(&good) && !styxshaperequest(17, &good) && !styxshapevalid(&empty_array) &&
        !styxshapevalid(&two_elements) && !styxshapevalid(&broken_ref) && !styxshapevalid(&too_deep);
}


/* EN: Check the separate admission rules for scalar values and copy-back refs.
 * RU: Проверить отдельные условия допуска скаляров и ref с обратной записью. */
static int dynamic_profile(void)
{
    const StyxCallShape good = {"((bi)ib)", "((b(ii)b))"};
    const StyxCallShape empty = {"()", "()"};
    const StyxCallShape too_wide = {"(iiiii)", "(b)"};
    const StyxCallShape reference = {"(r[ib])", "(i)"};
    const StyxCallShape aliases = {"((r[ib]r[ib]))", "()"};
    const StyxCallShape mixed = {"((r[ib]i))", "(i)"};
    const StyxCallShape wrong_fields = {"(r[bi])", "(i)"};
    const StyxCallShape wrong_result = {"(r[ib])", "(b)"};
    unsigned char bytes[STYX_SHAPED_OFFER_MAX];
    size_t a, r, used;
    return styxshapecells(&good, &a, &r) && a == 4 && r == 4 &&
        styxshapecells(&empty, &a, &r) && !a && !r &&
        !styxshapecells(&too_wide, &a, &r) && !styxshapecells(&reference, &a, &r) &&
        styxshapedoffer("sample", 17, &good, bytes, sizeof(bytes), &used) &&
        styxloadedaccept("sample", bytes, used) == STYX_DYNAMIC_REQUEST_TAG &&
        styxshaperef(&reference, &r) == STYX_REF_REQUEST_TAG && r == 1 &&
        styxshaperef(&aliases, &r) == STYX_REF_PAIR_REQUEST_TAG && r == 0 &&
        !styxshaperef(&mixed, &r) && !styxshaperef(&wrong_fields, &r) && !styxshaperef(&wrong_result, &r) &&
        !styxshaperequest(DVM_REF_PAIR_SIGNATURE, &aliases) &&
        styxshapedoffer("sample", 18, &aliases, bytes, sizeof(bytes), &used) &&
        styxloadedaccept("sample", bytes, used) == STYX_REF_PAIR_REQUEST_TAG;
}


/* EN: Check exact PID envelopes and the scoped profile's declared call schema.
 * RU: Проверить точную оболочку PID и объявленную схему контекстного профиля. */
static int scoped_call(void)
{
    const StyxCallShape shape = {"(ib)", "((ib))"};
    const StyxCallValues values = {.count = 2, .cells = {-4, INT64_MAX}};
    unsigned char raw[STYX_FUNCTION_FRAME_CAPACITY];
    unsigned char wrapped[STYX_SHAPED_OFFER_MAX];
    size_t raw_size, wrapped_size, length;
    uint32_t pid;
    const void *inside;
    if (!styxcallrecordwrite(STYX_DYNAMIC_REQUEST_TAG, shape.arguments, &values, raw, sizeof(raw), &raw_size) ||
        styxcall_scope_write(0, raw, raw_size, wrapped, sizeof(wrapped), &wrapped_size) ||
        !styxcall_scope_write(UINT32_MAX, raw, raw_size, wrapped, sizeof(wrapped), &wrapped_size) ||
        styxcall_scope_read(wrapped, wrapped_size - 1, &pid, &inside, &length) ||
        !styxcall_scope_read(wrapped, wrapped_size, &pid, &inside, &length) ||
        pid != UINT32_MAX || length != raw_size || memcmp(inside, raw, length))
    {
        return 0;
    }
    FVM_SRecord offer;
    size_t used;
    int caller = 0;
    StyxCallShape decoded;
    return styxshapedoffer("sample", 17, &shape, wrapped, sizeof(wrapped), &wrapped_size) &&
        fvm_srecord_read(wrapped, wrapped_size, sizeof(wrapped), &offer, &used) == FVM_S_OK &&
        fvm_srecord_write(STYX_SCOPED_OFFER_TAG, offer.payload, offer.size, raw, sizeof(raw), &raw_size) == FVM_S_OK &&
        styxscalar_profile_match_shape(raw, raw_size, "sample", 17, &caller, &decoded) && caller &&
        !strcmp(shape.arguments, decoded.arguments) && !strcmp(shape.result, decoded.result);
}


/* EN: Check that the scoped offer accepts shaped refs and rejects a fixed profile.
 * RU: Проверить допуск ref по схеме и отказ фиксированного контекстного профиля. */
static int scoped_ref_profile(void)
{
    const StyxCallShape shape = {"((r[ib]r[ib]))", "()"};
    const StyxCallShape fixed = {"(r[ib]r[ib])", "((ib))"};
    unsigned char owner[STYX_SHAPED_OFFER_MAX], published[STYX_SHAPED_OFFER_MAX];
    FVM_SRecord offer;
    size_t size, used;
    int caller = 0, explicit_end = 0;
    StyxCallShape decoded;
    return styxshapedoffer("sample", 18, &shape, owner, sizeof(owner), &size) &&
        fvm_srecord_read(owner, size, sizeof(owner), &offer, &used) == FVM_S_OK &&
        fvm_srecord_write(STYX_SCOPED_END_OFFER_TAG, offer.payload, offer.size, published, sizeof(published), &size) == FVM_S_OK &&
        styxscalar_profile_match_protocol(published, size, "sample", 18, &caller, &decoded, &explicit_end) &&
        caller && explicit_end &&
        !strcmp(shape.arguments, decoded.arguments) && !strcmp(shape.result, decoded.result) &&
        styxshapedoffer("sample", DVM_REF_PAIR_SIGNATURE, &fixed, owner, sizeof(owner), &size) &&
        fvm_srecord_read(owner, size, sizeof(owner), &offer, &used) == FVM_S_OK &&
        fvm_srecord_write(STYX_SCOPED_END_OFFER_TAG, offer.payload, offer.size, published, sizeof(published), &size) == FVM_S_OK &&
        !styxscalar_profile_match_shape(published, size, "sample", DVM_REF_PAIR_SIGNATURE, &caller, &decoded);
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
    uint32_t context_pid;
    if (!styxcall_context(".prog/.self", 11, &context_pid) || context_pid ||
        !styxcall_context(".prog/4294967295", 16, &context_pid) || context_pid != UINT32_MAX ||
        styxcall_context(".prog/4294967296", 16, &context_pid) ||
        styxcall_context(".prog/01", 8, &context_pid) ||
        styxcall_context(".prog/../9", 10, &context_pid))
    {
        return 1;
    }
    if (!reply_fields() || !managed_layout() || !managed_records() || !managed_profile() || !byte_record() || !real_record() || !dynamic_profile() || !shaped_profile() || !scoped_call() || !scoped_ref_profile())
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
