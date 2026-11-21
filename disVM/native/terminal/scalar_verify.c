/* Copyright (c) 2026 bhgv
 * EN: Scalar interface admission over existing Unibus record files.
 * RU: Допуск скалярного интерфейса через существующие файлы Unibus.
 */
#include "scalar_verify.h"
#include "flashvm/srecord.h"
#include <string.h>

/* EN/RU: Measure the borrowed nonempty name within the wire limit.
 * Измерить непустое заимствованное имя в пределах формата. */
size_t styxscalar_name_length(const char *name)
{
    size_t size = 0;
    if (name)
    {
        while (size <= INTERFACE_NAME_LIMIT && name[size])
        {
            size++;
        }
    }
    return size <= INTERFACE_NAME_LIMIT ? size : 0;
}


/* EN/RU: Encode a portable signature without host frame or pointer data.
 * Закодировать сигнатуру без данных нативного кадра и указателей. */
void styxscalar_interface_header(unsigned char *data, uint32_t signature)
{
    data[0] = INTERFACE_KIND;
    for (size_t i = INTERFACE_HEADER - 1; i > 0; i--)
    {
        data[i] = (unsigned char)signature;
        signature >>= 8;
    }
}


/* EN/RU: See scalar_interface.h; admit exactly one canonical S-record.
 * См. scalar_interface.h; допустить ровно одну каноническую S-запись. */
int styxscalaraccept(unsigned long tag, const void *bytes, size_t size,
    const char *name, uint32_t signature)
{
    size_t length = styxscalar_name_length(name);
    unsigned char data[INTERFACE_HEADER];
    FVM_SRecord record;
    size_t used;
    styxscalar_interface_header(data, signature);
    return tag && length &&
        fvm_srecord_read(bytes, size, INTERFACE_HEADER + INTERFACE_NAME_LIMIT + FVM_SRECORD_INTEGER_MAX,
            &record, &used) == FVM_S_OK &&
        size == used && record.tag == tag && record.size == sizeof(data) + length &&
        !memcmp(record.payload, data, sizeof(data)) &&
        !memcmp(record.payload + sizeof(data), name, length);
}


/* EN/RU: Check the complete framed sequence and identify one unambiguous match.
 * Проверить всю последовательность кадров и найти единственное совпадение. */
unsigned styxscalar_profile_match(const void *bytes, size_t size, const char *name, uint32_t signature, int *caller)
{
    const unsigned char *cursor = bytes;
    unsigned match = 0;
    int scoped = 0;
    for (unsigned i = 0; size && i < STYX_MOUNTED_EXPORT_LIMIT; i++)
    {
        FVM_SRecord record;
        size_t used;
        if (fvm_srecord_read(cursor, size, STYX_SCALAR_INTERFACE_CAPACITY, &record, &used) != FVM_S_OK ||
            (record.tag != STYX_FUNCTION_PROFILE_TAG && record.tag != STYX_FUNCTION_CALLER_PROFILE) || record.size <= INTERFACE_HEADER ||
            record.size > INTERFACE_HEADER + INTERFACE_NAME_LIMIT || record.payload[0] != INTERFACE_KIND)
        {
            return 0;
        }
        if (styxscalaraccept(record.tag, cursor, used, name, signature))
        {
            if (match)
            {
                return 0;
            }
            match = 1U << i;
            scoped = record.tag == STYX_FUNCTION_CALLER_PROFILE;
        }
        cursor += used;
        size -= used;
    }
    if (!size && match && caller)
    {
        *caller = scoped;
    }
    return size ? 0 : match;
}



/* EN/RU: See scalar_verify.h; typed tags fix the scalar width on the wire.
 * См. scalar_verify.h; типизированный тег фиксирует ширину в записи. */
uint32_t styxscalar_request_signature(unsigned long tag)
{
    switch (tag)
    {
    case STYX_FUNCTION_REQUEST_TAG:
    case STYX_FUNCTION_CALLER_TAG:
        return DVM_SCALAR_INT_SIGNATURE;
    case STYX_FUNCTION_BIG_TAG:
    case STYX_FUNCTION_BIG_CALLER_TAG:
        return DVM_SCALAR_BIG_SIGNATURE;
    case STYX_REF_PAIR_REQUEST_TAG:
        return DVM_REF_PAIR_SIGNATURE;
    case STYX_CALL_LIST_REQUEST_TAG:
        return DVM_CALL_LIST_SIGNATURE;
    case STYX_REF_REQUEST_TAG:
    case STYX_REF_CALLER_TAG:
        return DVM_REF_SIGNATURE;
    case STYX_TUPLE_REQUEST_TAG:
    case STYX_TUPLE_CALLER_TAG:
        return DVM_TUPLE_SIGNATURE;
    default:
        return 0;
    }
}



/* EN/RU: See scalar_verify.h. / См. scalar_verify.h. */
unsigned long styxfunction_reply_tag(unsigned long request)
{
    uint32_t signature = styxscalar_request_signature(request);
    return signature == DVM_REF_PAIR_SIGNATURE ? STYX_REF_PAIR_REPLY_TAG :
        signature == DVM_CALL_LIST_SIGNATURE ? STYX_CALL_LIST_REPLY_TAG :
        signature == DVM_REF_SIGNATURE ? STYX_REF_REPLY_TAG :
        signature == DVM_TUPLE_SIGNATURE ? STYX_TUPLE_REPLY_TAG : signature ? STYX_FUNCTION_REPLY_TAG : 0;
}
