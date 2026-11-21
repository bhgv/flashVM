/* Copyright (c) 2026 bhgv
 * EN: Decode bounded, receiver-local strings and byte arrays without cross-heap pointers.
 * RU: Декодирование строк и массивов байтов в локальной куче получателя.
 */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "managed_value.h"
#include "flashvm/utf8.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include <string.h>


/* EN: Consume one verified byte child, advancing the borrowed input on success.
 * RU: Прочитать один байт и продвинуть заимствованный вход при успехе. */
static int take_byte(const unsigned char **data, size_t *size, unsigned char *byte)
{
    FVM_SRecord child;
    size_t used;
    StyxScalarKind kind;
    int64_t value;
    if (fvm_srecord_read(*data, *size, STYX_MANAGED_LIST_LIMIT, &child, &used) != FVM_S_OK ||
        child.tag != STYX_CALL_BYTE_TAG ||
        !styxscalarread(STYX_CALL_BYTE_TAG, *data, used, &kind, &value) ||
        kind != STYX_SCALAR_VALUE || value < 0 || value > UINT8_MAX)
    {
        return 0;
    }
    *byte = (unsigned char)value;
    *data += used;
    *size -= used;
    return 1;
}


/* [GC:] EN: Check all bytes before allocation. Copy into one new unshared array.
 * RU: Проверить все байты до выделения. Скопировать в один новый массив. */
static int new_byte_array(const FVM_SRecord *record, void **result)
{
    const unsigned char *data = record->payload;
    size_t remaining = record->size;
    int length = 0;
    unsigned char byte;
    if (record->tag != STYX_MANAGED_ARRAY_TAG)
    {
        return 0;
    }
    while (remaining)
    {
        if (!take_byte(&data, &remaining, &byte))
        {
            return 0;
        }
        length++;
    }
    Array *array = mem2array(NULL, length);
    data = record->payload;
    remaining = record->size;
    for (int i = 0; i < length; i++)
    {
        if (!take_byte(&data, &remaining, &array->data[i]))
        {
            destroy(array);
            return 0;
        }
    }
    *result = array;
    return 1;
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_managed_new(const char *type, const FVM_SRecord *record, void **result)
{
    if (!type || !record || !result || record->size > STYX_MANAGED_LIST_LIMIT ||
        (!record->payload && record->size))
    {
        return 0;
    }
    int string = !strcmp(type, "s");
    int bytes = !strcmp(type, "a[y]");
    if (!string && !bytes)
    {
        return 0;
    }
    if (record->tag == STYX_MANAGED_NIL_TAG && !record->size)
    {
        *result = H;
        return 1;
    }
    if (bytes)
    {
        return new_byte_array(record, result);
    }
    if (record->tag != STYX_MANAGED_STRING_TAG ||
        fvm_utf8_validate((const char *)record->payload, record->size, NULL) != FVM_UTF8_OK)
    {
        return 0;
    }
    *result = c2string((char *)record->payload, (int)record->size);
    return 1;
}
