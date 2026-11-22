/* Copyright (c) 2026 bhgv
 * [GC:] EN: Marshal homogeneous primitive arrays without sharing heap pointers.
 * RU: Передавать однородные примитивные массивы без межкучевых указателей.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "managed_array.h"
#include "shared_bytes.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Resolve one supported primitive array element type to the VM's
 * canonical descriptor. Nested and pointer-bearing arrays need owned dynamic
 * descriptors and are deliberately refused here.
 * RU: Получить канонический дескриптор примитивного элемента массива. Для
 * вложенных массивов и указателей нужны собственные динамические типы.
 */
static Type *primitive_array_type(char element)
{
    switch (element)
    {
    case 'i':
        return &Tword;
    case 'b':
        return &Tlong;
    case 'f':
        return &Treal;
    case 'y':
        return &Tbyte;
    default:
        return NULL;
    }
}


/* EN/RU: See managed_array.h. / См. managed_array.h. */
int dvm_styx_primitive_array_supported(char element)
{
    return primitive_array_type(element) != NULL;
}


/* EN: Decode one scalar array element from a complete child record.
 * RU: Декодировать один скалярный элемент из полной дочерней записи.
 */
static int primitive_array_value(char element, const unsigned char *bytes, size_t size,
    size_t *used, int64_t *value)
{
    FVM_SRecord child;
    if (fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &child, used) != FVM_S_OK ||
        !*used || *used > size)
    {
        return 0;
    }
    unsigned long tag = element == 'i' ? STYX_CALL_INT_TAG :
        element == 'b' ? STYX_CALL_BIG_TAG : element == 'f' ? STYX_CALL_REAL_TAG :
        element == 'y' ? STYX_CALL_BYTE_TAG : 0;
    StyxScalarKind kind;
    return tag && styxscalarread(tag, bytes, *used, &kind, value) && kind == STYX_SCALAR_VALUE &&
        (element != 'i' || (*value >= INT32_MIN && *value <= INT32_MAX)) &&
        (element != 'y' || (*value >= 0 && *value <= UINT8_MAX));
}


/* [GC:] EN: Check each child before allocation, then copy into one owned array.
 * RU: Проверить все дочерние записи до выделения и создать один свой массив.
 */
int dvm_styx_primitive_array_new(char element, const FVM_SRecord *record, void **result)
{
    const unsigned char *data;
    size_t remaining, length = 0;
    Type *type = primitive_array_type(element);
    if (!record || !result || !type || record->tag != STYX_MANAGED_ARRAY_TAG || type->size <= 0)
    {
        return 0;
    }
    data = record->payload;
    remaining = record->size;
    while (remaining)
    {
        size_t used = 0;
        int64_t value;
        if (length == INT_MAX || !primitive_array_value(element, data, remaining, &used, &value))
        {
            return 0;
        }
        length++;
        data += used;
        remaining -= used;
    }
    Array *array = H2D(Array *, heaparray(type, (int)length));
    data = record->payload;
    remaining = record->size;
    for (size_t i = 0; i < length; i++)
    {
        size_t used = 0;
        int64_t value;
        if (!primitive_array_value(element, data, remaining, &used, &value))
        {
            destroy(array);
            return 0;
        }
        unsigned char *destination = array->data + i * (size_t)type->size;
        if (element == 'y')
        {
            *destination = (unsigned char)value;
        }
        else if (element == 'i')
        {
            WORD word = (WORD)value;
            memcpy(destination, &word, sizeof(word));
        }
        else if (element == 'b')
        {
            LONG wide = (LONG)value;
            memcpy(destination, &wide, sizeof(wide));
        }
        else
        {
            memcpy(destination, &value, sizeof(value));
        }
        data += used;
        remaining -= used;
    }
    *result = array;
    return 1;
}


/* [GC:] EN: Encode one element using the tag selected by its compiled type.
 * RU: Кодировать один элемент тегом, соответствующим типу компиляции.
 */
static int write_primitive_element(char element, const unsigned char *data, void *bytes,
    size_t capacity, size_t *used)
{
    int64_t value;
    unsigned long tag;
    if (element == 'y')
    {
        value = *data;
        tag = STYX_CALL_BYTE_TAG;
    }
    else if (element == 'i')
    {
        WORD word;
        memcpy(&word, data, sizeof(word));
        value = word;
        tag = STYX_CALL_INT_TAG;
    }
    else if (element == 'b')
    {
        LONG wide;
        memcpy(&wide, data, sizeof(wide));
        value = wide;
        tag = STYX_CALL_BIG_TAG;
    }
    else if (element == 'f')
    {
        memcpy(&value, data, sizeof(value));
        tag = STYX_CALL_REAL_TAG;
    }
    else
    {
        return 0;
    }
    return styxscalarwrite(tag, STYX_SCALAR_VALUE, value, bytes, capacity, used);
}


/* [GC:] EN: Snapshot and encode every homogeneous child into one array record.
 * RU: Снять копию и закодировать однородные дочерние записи в массив.
 */
int dvm_styx_primitive_array_write(char element, void *value, void *bytes,
    size_t capacity, size_t *used)
{
    Type *type = primitive_array_type(element);
    if (!value || !bytes || !used || !capacity || !type || D2H(value)->t != &Tarray)
    {
        return 0;
    }
    if (capacity > STYX_MANAGED_LIST_LIMIT)
    {
        capacity = STYX_MANAGED_LIST_LIMIT;
    }
    Array *array = value;
    if (!array->t || array->t == H || array->t->size != type->size || array->t->np ||
        array->len < 0 || type->size <= 0 ||
        (size_t)array->len > capacity / (size_t)type->size)
    {
        return 0;
    }
    size_t length = (size_t)array->len;
    size_t extent = length * (size_t)type->size;
    unsigned char *snapshot = extent ? malloc(extent) : NULL;
    unsigned char *payload = length ? malloc(capacity) : NULL;
    int ok = (!extent || snapshot) && (!length || payload) &&
        !dvm_runtime_bytes_snapshot(array->data, snapshot, extent);
    size_t size = 0;
    for (size_t i = 0; ok && i < length; i++)
    {
        size_t child;
        ok = write_primitive_element(element, snapshot + i * (size_t)type->size,
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
