/* Copyright (c) 2026 bhgv
 * [GC:] EN: Marshal homogeneous arrays using scalar or mapped element layouts.
 * RU: Передавать однородные массивы со скалярными или размеченными элементами.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "managed_array.h"
#include "shared_bytes.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Resolve one supported primitive array element type to the VM's
 * canonical descriptor for the primitive fast path.
 * RU: Получить канонический дескриптор примитивного элемента для быстрого пути.
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


/* EN: Convert the element layout to the Dis descriptor's exact pointer map.
 * RU: Преобразовать раскладку элемента в точную карту ссылок Dis. */
static unsigned char element_map(const StyxScalarLayout *layout)
{
    unsigned char map = 0;
    for (size_t i = 0; i < layout->count; i++)
    {
        if (layout->pointers & (1U << i))
        {
            map |= 0x80U >> (layout->offset[i] / sizeof(void *));
        }
    }
    return map;
}


typedef struct CompositeDecode
{
    const FVM_SRecord *record; /* Borrowed complete array record. */
    const char *schema; /* Borrowed singleton schema. */
    const StyxScalarLayout *layout; /* Layout of one element. */
    Type *type; /* Borrowed descriptor, retained by the constructed array. */
    int length; /* Count of length-checked child records. */
    Array *array; /* Partially built array, owned by caller. */
    int rooted; /* Whether array has a temporary GC root. */
} CompositeDecode;


/* [GC:] EN: Allocate the array and install each decoded element into its mapped slot.
 * RU: Выделить массив и записать каждый декодированный элемент в размеченный слот. */
static int decode_composite(void *state)
{
    CompositeDecode *decode = state;
    decode->array = H2D(Array *, heaparray(decode->type, decode->length));
    ptradd(D2H(decode->array));
    decode->rooted = 1;
    const unsigned char *bytes = decode->record->payload;
    size_t remaining = decode->record->size;
    for (int i = 0; i < decode->length; i++)
    {
        FVM_SRecord record;
        size_t used;
        DVM_StyxValueFrame element = {0};
        if (fvm_srecord_read(bytes, remaining, STYX_MANAGED_LIST_LIMIT, &record, &used) != FVM_S_OK ||
            !dvm_styx_array_element_read(decode->schema, bytes, used, decode->layout, &element))
        {
            return 0;
        }
        unsigned char *slot = decode->array->data + (size_t)i * decode->layout->size;
        if (!dvm_styx_value_frame_move(&element, slot, decode->layout->size))
        {
            dvm_styx_value_frame_clear(&element);
            return 0;
        }
        bytes += used;
        remaining -= used;
    }
    return remaining == 0;
}


/* EN/RU: See managed_array.h. / См. managed_array.h. */
int dvm_styx_composite_array_new(const char *type, const FVM_SRecord *record, void **result)
{
    char schema[STYX_SHAPE_MAX + 1];
    StyxScalarLayout layout;
    if (!record || record->tag != STYX_MANAGED_ARRAY_TAG || !result ||
        !styxmanaged_array_layout(type, schema, &layout))
    {
        return 0;
    }
    int length = 0;
    for (size_t offset = 0; offset < record->size; length++)
    {
        FVM_SRecord child;
        size_t used;
        if (fvm_srecord_read(record->payload + offset, record->size - offset,
            STYX_MANAGED_LIST_LIMIT, &child, &used) != FVM_S_OK)
        {
            return 0;
        }
        offset += used;
    }
    unsigned char map = element_map(&layout);
    Type *descriptor = layout.count == 1 && map == 0x80 ? &Tptr :
        dtype(freeheap, (int)layout.size, &map, map != 0);
    if (!descriptor)
    {
        return 0;
    }
    CompositeDecode decode = {.record = record, .schema = schema, .layout = &layout,
        .type = descriptor, .length = length};
    int valid = kprotect(decode_composite, &decode);
    if (decode.rooted)
    {
        ptrdel(D2H(decode.array));
    }
    if (descriptor != &Tptr)
    {
        freetype(descriptor);
    }
    if (!valid)
    {
        if (decode.array)
        {
            destroy(decode.array);
        }
        return 0;
    }
    *result = decode.array;
    return 1;
}


typedef struct CompositeEncode
{
    const char *schema; /* Borrowed checked element schema. */
    DVM_StyxValueFrame *element; /* Temporarily retained fields of one element. */
    unsigned char *scratch; /* Shared tuple scratch for this array level. */
    unsigned char *bytes; /* Destination of one element record. */
    size_t capacity; /* Bytes available for this record. */
    size_t used; /* Bytes written on success. */
} CompositeEncode;


/* [GC:] EN: Root the retained cells while encoding their nested records.
 * RU: Укоренить удерживаемые ссылки при кодировании вложенных записей. */
static int encode_composite_element(void *state)
{
    CompositeEncode *encode = state;
    DVM_StyxValueFrame *element = encode->element;
    for (size_t i = 0; i < element->layout.count; i++)
    {
        if (element->layout.pointers & (1U << i))
        {
            void *value;
            memcpy(&value, (unsigned char *)element->words + element->layout.offset[i], sizeof(value));
            if (value != H)
            {
                ptradd(D2H(value));
                element->roots |= 1U << i;
                Setmark(D2H(value));
            }
        }
    }
    return dvm_styx_array_element_write(encode->schema, element, encode->scratch,
        encode->bytes, encode->capacity, &encode->used);
}


/* EN: Compare each map byte, allowing compiler descriptors with zero padding.
 * RU: Сверить карту, учитывая нулевые байты дополнения дескриптора компилятора. */
static int array_element_matches(const Type *type, const StyxScalarLayout *layout)
{
    if (!type || type == H || type->size != (int)layout->size || type->np < 0 ||
        type->np > (type->size + 63) / 64)
    {
        return 0;
    }
    unsigned char map = element_map(layout);
    return (type->np ? type->map[0] : 0) == map;
}


/* EN/RU: See managed_array.h. / См. managed_array.h. */
int dvm_styx_composite_array_write(const char *type, void *value, void *bytes, size_t capacity, size_t *used)
{
    char schema[STYX_SHAPE_MAX + 1];
    StyxScalarLayout layout;
    if (!value || value == H || D2H(value)->t != &Tarray ||
        !styxmanaged_array_layout(type, schema, &layout))
    {
        return 0;
    }
    Array *array = value;
    if (array->len < 0 || (size_t)array->len > capacity ||
        !array_element_matches(array->t, &layout))
    {
        return 0;
    }
    if (!array->len)
    {
        return fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    int tuples = schema[1] == '(';
    size_t regions = tuples ? 1 + DVM_STYX_TUPLE_SCRATCH : 1;
    unsigned char *payload = malloc(regions * capacity);
    if (!payload)
    {
        return 0;
    }
    size_t length = 0;
    int valid = 1;
    for (int i = 0; valid && i < array->len; i++)
    {
        DVM_StyxValueFrame element = {.layout = layout};
        CompositeEncode encode = {.schema = schema, .element = &element,
            .scratch = tuples ? payload + capacity : NULL,
            .bytes = payload + length, .capacity = capacity - length};
        valid = encode.capacity > 0 &&
            dvm_styx_value_snapshot(array->data + (size_t)i * layout.size, &element) &&
            kprotect(encode_composite_element, &encode);
        dvm_styx_value_frame_clear(&element);
        length += encode.used;
    }
    valid = valid && fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, payload, length, bytes, capacity, used) == FVM_S_OK;
    free(payload);
    return valid;
}
