/* Copyright (c) 2026 bhgv
 * EN: Portable fixed-width scalar payload for one authorized Unibus method route.
 * RU: Переносимые скалярные данные фиксированной ширины для выданного маршрута Unibus.
 */
#include "scalar_record.h"
#include "scalar_verify.h"
#include "flashvm/srecord.h"


/* EN/RU: See scalar_record.h; the route tag is independent of scalar kind.
 * См. scalar_record.h; тег маршрута независим от вида скалярной записи. */
int styxscalarwrite(unsigned long tag, StyxScalarKind kind, int64_t value,
    void *bytes, size_t capacity, size_t *used)
{
    if (!tag || tag > FVM_SRECORD_U32 ||
        kind < STYX_SCALAR_CALL || kind > STYX_SCALAR_ERROR || !bytes || !used)
    {
        return 0;
    }
    unsigned char payload[9]; /* Kind followed by signed 64-bit two's-complement bits. */
    uint64_t bits = (uint64_t)value;
    payload[0] = (unsigned char)kind;
    for (size_t i = 8; i > 0; i--)
    {
        payload[i] = (unsigned char)bits;
        bits >>= 8;
    }
    return fvm_srecord_write(tag, payload, sizeof(payload), bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See scalar_record.h; validate before publishing either scalar output.
 * См. scalar_record.h; проверить запись до выдачи обоих выходных значений. */
int styxscalarread(unsigned long tag, const void *bytes, size_t size,
    StyxScalarKind *kind, int64_t *value)
{
    FVM_SRecord record;
    size_t consumed;
    if (!tag || tag > FVM_SRECORD_U32 || !bytes || !kind || !value ||
        fvm_srecord_read(bytes, size, 9 + FVM_SRECORD_INTEGER_MAX, &record, &consumed) != FVM_S_OK ||
        consumed != size || record.tag != tag || record.size != 9 ||
        record.payload[0] < STYX_SCALAR_CALL || record.payload[0] > STYX_SCALAR_ERROR)
    {
        return 0;
    }
    uint64_t bits = 0;
    for (size_t i = 1; i <= 8; i++)
    {
        bits = (bits << 8) | record.payload[i];
    }
    *kind = (StyxScalarKind)record.payload[0];
    *value = bits <= INT64_MAX ? (int64_t)bits : -1 - (int64_t)(UINT64_MAX - bits);
    return 1;
}


/* EN/RU: Validate the entire request and reply capacity before invoking the method.
 * Проверить запрос целиком и размер буфера до вызова метода. */
int styxscalarrespond(unsigned long request_tag, const void *request, size_t request_size,
    unsigned long reply_tag, int (*method)(void *, int64_t, int64_t *), void *state,
    void *reply, size_t capacity, size_t *used)
{
    StyxScalarKind kind;
    int64_t argument;
    if (!reply_tag || reply_tag > FVM_SRECORD_U32 || !method || !reply || !used ||
        capacity < STYX_SCALAR_RECORD_CAPACITY ||
        !styxscalarread(request_tag, request, request_size, &kind, &argument) || kind != STYX_SCALAR_CALL)
    {
        return 0;
    }
    int64_t result = 0;
    int success = method(state, argument, &result);
    return styxscalarwrite(reply_tag, success == 1 ? STYX_SCALAR_VALUE : STYX_SCALAR_ERROR,
        result, reply, capacity, used);
}


/* EN/RU: Write exactly two big-endian words, with PID first.
 * Записать ровно два слова старшим байтом вперёд, первым PID. */
int styxscalarcallerwrite(uint32_t pid, int32_t argument, void *bytes, size_t capacity, size_t *used)
{
    if (!pid || !bytes || !used)
    {
        return 0;
    }
    unsigned char payload[8];
    const uint32_t fields[] = {pid, (uint32_t)argument};
    for (unsigned i = 0; i < sizeof(payload); i++)
    {
        payload[i] = (unsigned char)(fields[i / 4] >> (24 - 8 * (i % 4)));
    }
    return fvm_srecord_write(STYX_FUNCTION_CALLER_TAG, payload, sizeof(payload), bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: Reject malformed or anonymous requests without writing the borrowed outputs.
 * Отклонить неверный или анонимный запрос, сохранив выходные значения. */
int styxscalarcallerread(const void *bytes, size_t size, uint32_t *pid, int64_t *argument)
{
    FVM_SRecord record;
    size_t used;
    if (!bytes || !pid || !argument ||
        fvm_srecord_read(bytes, size, STYX_SCALAR_RECORD_CAPACITY, &record, &used) != FVM_S_OK ||
        used != size || record.tag != STYX_FUNCTION_CALLER_TAG || record.size != 8)
    {
        return 0;
    }
    uint32_t fields[2] = {0};
    for (unsigned i = 0; i < 8; i++)
    {
        fields[i / 4] = (fields[i / 4] << 8) | record.payload[i];
    }
    if (!fields[0])
    {
        return 0;
    }
    *pid = fields[0];
    *argument = fields[1] <= INT32_MAX ? (int64_t)fields[1] : (int64_t)fields[1] - INT64_C(4294967296);
    return 1;
}
