/* Copyright (c) 2026 bhgv
 * EN: Validate an ordered tuple of typed nested S-records.
 * RU: Проверка упорядоченного кортежа типизированных вложенных S-записей.
 */
#include "tuple_record.h"
#include "call_record.h"
#include "scalar_verify.h"
#include "flashvm/srecord.h"

/* EN: Select the existing wire shape for a supported tuple or ref tag.
 * RU: Выбрать прежнюю форму записи для поддерживаемого тега кортежа или ref. */
static const char *tuple_shape(unsigned long tag)
{
    switch (tag)
    {
    case STYX_TUPLE_CALLER_TAG:
    case STYX_REF_CALLER_TAG:
        return "(pib)";
    case STYX_TUPLE_REQUEST_TAG:
    case STYX_TUPLE_REPLY_TAG:
    case STYX_REF_REQUEST_TAG:
        return "(ib)";
    default:
        return NULL;
    }
}


/* EN/RU: See tuple_record.h. / См. tuple_record.h. */
int styxtuplewrite(unsigned long tag, uint32_t pid, const StyxTuple *value,
    void *bytes, size_t capacity, size_t *used)
{
    const char *shape = tuple_shape(tag);
    if (!shape || !value || (!!pid != (shape[1] == 'p')))
    {
        return 0;
    }
    const StyxCallValues fields = pid ? (StyxCallValues){3, {pid, value->narrow, value->wide}} :
        (StyxCallValues){2, {value->narrow, value->wide}};
    return styxcallrecordwrite(tag, shape, &fields, bytes, capacity, used);
}


/* EN/RU: See tuple_record.h. / См. tuple_record.h. */
int styxtupleread(unsigned long tag, const void *bytes, size_t size, uint32_t *pid, StyxTuple *value)
{
    StyxCallValues fields;
    if (!pid || !value || !styxcallrecordread(tag, tuple_shape(tag), bytes, size, &fields))
    {
        return 0;
    }
    size_t first = fields.count - 2;
    *pid = first ? (uint32_t)fields.cells[0] : 0;
    *value = (StyxTuple){(int32_t)fields.cells[first], fields.cells[first + 1]};
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
