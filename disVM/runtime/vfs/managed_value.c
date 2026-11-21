/* Copyright (c) 2026 bhgv
 * EN: Encode and decode bounded strings and byte arrays without cross-heap pointers.
 * RU: Кодирование и декодирование строк и байтовых массивов без межкучевых указателей.
 */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "managed_value.h"
#include "shared_bytes.h"
#include "flashvm/utf8.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include <string.h>
#include <stdlib.h>


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


/* [GC:] EN: Snapshot one counted ASCII string or convert copied native runes to UTF-8.
 * RU: Скопировать строку ASCII с её длиной либо преобразовать копию Rune в UTF-8. */
static int write_string(String *text, void *bytes, size_t capacity, size_t *used)
{
    int length = text->len;
    int wide = length < 0;
    size_t count = wide ? (size_t)(-(int64_t)length) : (size_t)length;
    if (text->max < 0 || count > (size_t)text->max || count > capacity)
    {
        return 0;
    }
    size_t extent = count * (wide ? sizeof(Rune) : 1);
    unsigned char *snapshot = extent ? malloc(extent) : NULL;
    unsigned char *utf8 = wide ? malloc(capacity) : snapshot;
    int ok = (!extent || snapshot) && (!wide || utf8) &&
        !dvm_runtime_bytes_snapshot(&text->data, snapshot, extent);
    size_t size = wide ? 0 : count;
    if (ok && wide)
    {
        for (size_t i = 0; ok && i < count; i++)
        {
            Rune rune;
            size_t width;
            memcpy(&rune, snapshot + i * sizeof(Rune), sizeof(rune));
            ok = fvm_utf8_write(rune, (char *)utf8 + size, capacity - size, &width) == FVM_UTF8_OK;
            if (ok)
            {
                size += width;
            }
        }
    }
    ok = ok && fvm_srecord_write(STYX_MANAGED_STRING_TAG, utf8, size, bytes, capacity, used) == FVM_S_OK;
    if (wide)
    {
        free(utf8);
    }
    free(snapshot);
    return ok;
}


/* [GC:] EN: Copy an array's live byte span once and encode each homogeneous child.
 * RU: Один раз скопировать живой диапазон массива и кодировать каждый элемент. */
static int write_array(Array *array, void *bytes, size_t capacity, size_t *used)
{
    if (array->t != &Tbyte || array->len < 0 || (size_t)array->len > capacity)
    {
        return 0;
    }
    size_t length = (size_t)array->len;
    unsigned char *snapshot = length ? malloc(length) : NULL;
    unsigned char *payload = length ? malloc(capacity) : NULL;
    int ok = (!length || (snapshot && payload)) &&
        !dvm_runtime_bytes_snapshot(array->data, snapshot, length);
    size_t size = 0;
    for (size_t i = 0; ok && i < length; i++)
    {
        size_t child;
        ok = styxscalarwrite(STYX_CALL_BYTE_TAG, STYX_SCALAR_VALUE, snapshot[i],
            payload + size, capacity - size, &child);
        if (ok)
        {
            size += child;
        }
    }
    ok = ok && fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, payload, size, bytes, capacity, used) == FVM_S_OK;
    free(payload);
    free(snapshot);
    return ok;
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_managed_write(const char *type, void *value, void *bytes, size_t capacity, size_t *used)
{
    if (!type || !value || !bytes || !used || !capacity)
    {
        return 0;
    }
    int string = !strcmp(type, "s");
    if (!string && strcmp(type, "a[y]"))
    {
        return 0;
    }
    if (capacity > STYX_MANAGED_LIST_LIMIT)
    {
        capacity = STYX_MANAGED_LIST_LIMIT;
    }
    if (value == H)
    {
        return fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    if (D2H(value)->t != (string ? &Tstring : &Tarray))
    {
        return 0;
    }
    return string ? write_string(value, bytes, capacity, used) : write_array(value, bytes, capacity, used);
}
