/* Copyright (c) 2026 bhgv
 * EN: Validate an ordered tuple of typed nested S-records.
 * RU: Проверка упорядоченного кортежа типизированных вложенных S-записей.
 */
#include "tuple_record.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include "flashvm/srecord.h"

#define FIELD_INT 1UL /* Signed int tuple member. */
#define FIELD_BIG 2UL /* Signed big tuple member. */
#define FIELD_PID 3UL /* Session-local process selector. */


/* EN: Append a signed field to the bounded temporary tuple buffer.
 * RU: Добавить знаковое поле в ограниченный временный буфер кортежа. */
static int append(unsigned long tag, int64_t value, unsigned char *bytes, size_t *size)
{
    size_t used;
    if (!styxscalarwrite(tag, STYX_SCALAR_VALUE, value, bytes + *size,
            STYX_FUNCTION_FRAME_CAPACITY - *size, &used))
    {
        return 0;
    }
    *size += used;
    return 1;
}


/* EN/RU: See tuple_record.h. / См. tuple_record.h. */
int styxtuplewrite(unsigned long tag, uint32_t pid, const StyxTuple *value,
    void *bytes, size_t capacity, size_t *used)
{
    unsigned char fields[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size = 0;
    int scoped = tag == STYX_TUPLE_CALLER_TAG || tag == STYX_REF_CALLER_TAG;
    return value && bytes && used &&
        (scoped ? pid && append(FIELD_PID, pid, fields, &size) :
            !pid && (tag == STYX_TUPLE_REQUEST_TAG || tag == STYX_TUPLE_REPLY_TAG || tag == STYX_REF_REQUEST_TAG)) &&
        append(FIELD_INT, value->narrow, fields, &size) && append(FIELD_BIG, value->wide, fields, &size) &&
        fvm_srecord_write(tag, fields, size, bytes, capacity, used) == FVM_S_OK;
}


/* EN: Read one ordered field and advance only after validating its tag and kind.
 * RU: Прочитать очередное поле и продвинуться после проверки тега и вида. */
static int field(unsigned long tag, const unsigned char **bytes, size_t *size, int64_t *value)
{
    FVM_SRecord record;
    size_t used;
    StyxScalarKind kind;
    if (fvm_srecord_read(*bytes, *size, STYX_SCALAR_RECORD_CAPACITY, &record, &used) != FVM_S_OK ||
        !styxscalarread(tag, *bytes, used, &kind, value) || kind != STYX_SCALAR_VALUE)
    {
        return 0;
    }
    *bytes += used;
    *size -= used;
    return 1;
}


/* EN/RU: See tuple_record.h. / См. tuple_record.h. */
int styxtupleread(unsigned long tag, const void *bytes, size_t size, uint32_t *pid, StyxTuple *value)
{
    FVM_SRecord record;
    size_t used;
    if (!bytes || !pid || !value ||
        fvm_srecord_read(bytes, size, STYX_FUNCTION_FRAME_CAPACITY, &record, &used) != FVM_S_OK ||
        used != size || record.tag != tag)
    {
        return 0;
    }
    const unsigned char *cursor = record.payload;
    size_t remaining = record.size;
    int64_t identity = 0, narrow, wide;
    int scoped = tag == STYX_TUPLE_CALLER_TAG || tag == STYX_REF_CALLER_TAG;
    if (!(scoped ? field(FIELD_PID, &cursor, &remaining, &identity) && identity > 0 && identity <= UINT32_MAX :
            tag == STYX_TUPLE_REQUEST_TAG || tag == STYX_TUPLE_REPLY_TAG || tag == STYX_REF_REQUEST_TAG) ||
        !field(FIELD_INT, &cursor, &remaining, &narrow) || narrow < INT32_MIN || narrow > INT32_MAX ||
        !field(FIELD_BIG, &cursor, &remaining, &wide) || remaining)
    {
        return 0;
    }
    *pid = (uint32_t)identity;
    *value = (StyxTuple){.narrow = (int32_t)narrow, .wide = wide};
    return 1;
}


/* EN: Put the stable return and updated ADT snapshots in one outer record.
 * RU: Поместить результат и обновлённый ADT в одну внешнюю запись. */
int styxrefreplywrite(const StyxRefReply *reply, void *bytes, size_t capacity, size_t *used)
{
    unsigned char payload[STYX_FUNCTION_FRAME_CAPACITY];
    size_t first, second;
    return reply && bytes && used &&
        styxtuplewrite(STYX_TUPLE_REPLY_TAG, 0, &reply->result, payload, sizeof(payload), &first) &&
        styxtuplewrite(STYX_TUPLE_REPLY_TAG, 0, &reply->updated, payload + first, sizeof(payload) - first, &second) &&
        fvm_srecord_write(STYX_REF_REPLY_TAG, payload, first + second, bytes, capacity, used) == FVM_S_OK;
}


/* EN: Validate both snapshots and exact framing before modifying the output.
 * RU: Сверить оба снимка и точный кадр до изменения результата. */
int styxrefreplyread(const void *bytes, size_t size, StyxRefReply *reply)
{
    FVM_SRecord outer, first;
    size_t used, length;
    uint32_t pid;
    StyxRefReply decoded;
    if (!reply || !bytes ||
        fvm_srecord_read(bytes, size, STYX_FUNCTION_FRAME_CAPACITY, &outer, &used) != FVM_S_OK ||
        used != size || outer.tag != STYX_REF_REPLY_TAG ||
        fvm_srecord_read(outer.payload, outer.size, STYX_FUNCTION_FRAME_CAPACITY, &first, &length) != FVM_S_OK ||
        !styxtupleread(STYX_TUPLE_REPLY_TAG, outer.payload, length, &pid, &decoded.result) ||
        !styxtupleread(STYX_TUPLE_REPLY_TAG, outer.payload + length, outer.size - length, &pid, &decoded.updated))
    {
        return 0;
    }
    *reply = decoded;
    return 1;
}
