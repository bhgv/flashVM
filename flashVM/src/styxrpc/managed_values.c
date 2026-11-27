/* Copyright (c) 2026 bhgv
 * EN: Translate managed by-value records without retaining wire pointers in FlashVM.
 * RU: Перевод управляемых записей по значению без указателей на транспорт в FlashVM.
 */
#include "managed_values.h"
#include "flashvm/execution.h"
#include "styxrpc/hybrid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FVM_STYXRPC_SAFE_BIG INT64_C(9007199254740991) /* Largest uniformly exact AS integer. */
#define FVM_STYXRPC_INDEX_BYTES 11U /* Decimal index plus NUL on 32-bit unsigned. */


/* EN: Map a scalar schema token to its established record tag.
 * RU: Сопоставить скалярный тип с его действующим тегом. */
static unsigned long integer_tag(char type)
{
    return type == 'i' ? STYXRPC_VALUE_INT : type == 'b' ? STYXRPC_VALUE_BIG : STYXRPC_VALUE_BYTE;
}


/* EN: Verify an exact numeric representation before converting to a signed integer.
 * RU: Проверить точное числовое представление перед преобразованием в целое. */
static int exact_integer(const FVM_Value *value, double low, double high, int64_t *result)
{
    int64_t integral;
    if (value->type != FVM_VALUE_NUMBER || !(value->number >= low && value->number <= high))
    {
        return 0;
    }
    integral = (int64_t)value->number;
    if ((double)integral != value->number)
    {
        return 0;
    }
    *result = integral;
    return 1;
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_read_big(const FVM_Value *value, int64_t *out)
{
    if (value->type == FVM_VALUE_STRING)
    {
        return styxrpc_hybrid_big_parse(value->string, fvm_value_string_size(value), out);
    }
    return exact_integer(value, -(double)FVM_STYXRPC_SAFE_BIG, (double)FVM_STYXRPC_SAFE_BIG, out);
}


/* [GC:] EN/RU: See managed_values.h. / См. managed_values.h. */
FVM_Result fvm_styxrpc_return_big(FVM_Context *context, int64_t number, FVM_Value *out)
{
    char bytes[STYXRPC_HYBRID_BIG_TEXT];
    unsigned short units[STYXRPC_HYBRID_BIG_TEXT];
    size_t size, i;
    if (number >= -FVM_STYXRPC_SAFE_BIG && number <= FVM_STYXRPC_SAFE_BIG)
    {
        fvm_value_number(out, (double)number);
        return FVM_OK;
    }
    size = styxrpc_hybrid_big_format(number, bytes, sizeof(bytes));
    for (i = 0; i < size; ++i)
    {
        units[i] = (unsigned char)bytes[i];
    }
    return size ? fvm_value_string_utf16(context, units, size, out) : FVM_ERROR;
}


/* EN: Encode a checked scalar, string or compound value to the supplied output.
 * RU: Закодировать проверенный скаляр, строку или составное значение в буфер. */
static int encode_value(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *type,
    const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used);


/* [GC:] EN: Snapshot listed indices under roots while encoding a compound.
 * RU: Снять значения индексов под корнями при кодировании составного значения. */
static int encode_compound(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *type,
    const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    FVM_Value length, element;
    int64_t count;
    const char *child = type + ((*type == 'a' || *type == 'l') ? 2 : 1);
    unsigned char *body;
    size_t size = 0, placed;
    unsigned i;
    int present, encoded;
    if (value->type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, value->object, "length", &length) != FVM_OK ||
        !exact_integer(&length, 0, FVM_STYXRPC_ARRAY_ITEMS, &count))
    {
        return 0;
    }
    if (*type == 'l' && !count)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    body = malloc(capacity ? capacity : 1);
    if (!body)
    {
        return 0;
    }
    for (i = 0; i < (unsigned)count; ++i)
    {
        char index[FVM_STYXRPC_INDEX_BYTES];
        sprintf(index, "%u", i);
        if (*child == ')' || fvm_object_has_own(context, value->object, index, &present) != FVM_OK ||
            !present || fvm_object_get(context, value->object, index, &element) != FVM_OK ||
            fvm_gc_roots_add(roots, &element) != FVM_OK ||
            !encode_value(context, roots, (*type == 'a' || *type == 'l') && (!refs || !refs->graph) ? NULL : refs, child, &element, body + size, capacity - size, &placed))
        {
            break;
        }
        size += placed;
        if (*type == '(')
        {
            child = fvm_styxrpc_type_end(child);
        }
    }
    encoded = i == (unsigned)count && ((*type == 'a' || *type == 'l') || *child == ')') &&
        fvm_srecord_write((*type == 'a' || *type == 'l') ? STYXRPC_VALUE_ARRAY : STYXRPC_VALUE_TUPLE,
            body, size, bytes, capacity, used) == FVM_S_OK;
    free(body);
    return encoded;
}


/* EN/RU: See declaration above. / См. объявление выше. */
static int encode_value(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *type,
    const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    int64_t number;
    if (*type == 'c')
    {
        return refs && refs->encode_channel &&
            refs->encode_channel(context, roots, refs->channel_context, type, value, bytes, capacity, used);
    }
    if (*type == 'r' || *type == 'h')
    {
        return refs && (*type == 'r' || refs->graph) &&
            fvm_styxrpc_ref_encode(context, roots, refs, type, value, bytes, capacity, used);
    }
    if ((*type == 's' || *type == 'a' || *type == 'l') && value->type == FVM_VALUE_NULL)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    if (*type == 's')
    {
        return value->type == FVM_VALUE_STRING &&
            fvm_srecord_write(STYXRPC_VALUE_STRING, value->string, fvm_value_string_size(value),
                bytes, capacity, used) == FVM_S_OK;
    }
    if (*type == 'f')
    {
        return value->type == FVM_VALUE_NUMBER &&
            styxrpc_value_real(value->number, bytes, capacity, used) == STYXRPC_OK;
    }
    if (*type == '(' || *type == 'a' || *type == 'l')
    {
        return encode_compound(context, roots, refs, type, value, bytes, capacity, used);
    }
    if (*type == 'b')
    {
        return fvm_styxrpc_read_big(value, &number) &&
            styxrpc_value_integer(STYXRPC_VALUE_BIG, number, bytes, capacity, used) == STYXRPC_OK;
    }
    return exact_integer(value, INT32_MIN, INT32_MAX, &number) &&
        styxrpc_value_integer(integer_tag(*type), number, bytes, capacity, used) == STYXRPC_OK;
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_values_encode(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *shape,
    const FVM_Value *values, unsigned count, unsigned char *bytes, size_t capacity, size_t *used)
{
    const char *type = shape + 1;
    size_t size = 0, placed;
    unsigned i;
    for (i = 0; i < count; ++i)
    {
        if (*type == ')' || !encode_value(context, roots, refs, type, values + i, bytes + size,
            capacity - size, &placed))
        {
            return 0;
        }
        size += placed;
        type = fvm_styxrpc_type_end(type);
    }
    *used = size;
    return *type == ')';
}


/* [GC:] EN/RU: See managed_values.h. / См. managed_values.h. */
FVM_Result fvm_styxrpc_return_text(FVM_Context *context, const char *bytes, size_t size, FVM_Value *value)
{
    FVM_Value borrowed;
    size_t count;
    unsigned short *units = malloc((size ? size : 1) * sizeof(*units));
    FVM_Result status;
    if (!units)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_value_string_bytes(&borrowed, bytes, size);
    if (status == FVM_OK)
    {
        status = fvm_value_get_utf16(&borrowed, units, size, &count);
    }
    if (status == FVM_OK)
    {
        status = fvm_value_string_utf16(context, units, count, value);
    }
    free(units);
    return status;
}


/* [GC:] EN: Decode one bounded record and root any fresh context value.
 * RU: Декодировать одну ограниченную запись и удержать новое значение контекста. */
static int decode_value(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *type,
    FVM_SRecord *cursor, FVM_Value *value);


/* [GC:] EN: Materialize a compound as a dense AS Array with rooted elements.
 * RU: Собрать составное значение как плотный Array AS с корнями элементов. */
static int decode_compound(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *type,
    FVM_SRecord record, FVM_Value *value)
{
    FVM_SRecord scan = record, child;
    size_t used;
    unsigned count = 0, i;
    const char *shape = type + ((*type == 'a' || *type == 'l') ? 2 : 1);
    FVM_Value *items;
    int ok;
    while (scan.size)
    {
        if (count == FVM_STYXRPC_ARRAY_ITEMS ||
            fvm_srecord_read(scan.payload, scan.size, STYXRPC_MANAGED_LIMIT, &child, &used) != FVM_S_OK)
        {
            return 0;
        }
        ++count;
        scan.payload += used;
        scan.size -= used;
    }
    items = malloc((count ? count : 1) * sizeof(*items));
    if (!items)
    {
        return 0;
    }
    for (i = 0; i < count; ++i)
    {
        if (*shape == ')' || !decode_value(context, roots, (*type == 'a' || *type == 'l') && (!refs || !refs->graph) ? NULL : refs, shape, &record, items + i))
        {
            break;
        }
        if (*type == '(')
        {
            shape = fvm_styxrpc_type_end(shape);
        }
    }
    ok = i == count && ((*type == 'a' || *type == 'l') || *shape == ')') &&
        fvm_exec_literal_array(context, count, items, value) == FVM_OK;
    free(items);
    return ok;
}


/* [GC:] EN/RU: See declaration above. / См. объявление выше. */
static int decode_value(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *type,
    FVM_SRecord *cursor, FVM_Value *value)
{
    FVM_SRecord node;
    int64_t integer;
    const unsigned char *bytes = cursor->payload;
    size_t used;
    if (fvm_srecord_read(cursor->payload, cursor->size, STYXRPC_MANAGED_LIMIT, &node, &used) != FVM_S_OK)
    {
        return 0;
    }
    cursor->payload += used;
    cursor->size -= used;
    if (*type == 'c')
    {
        return refs && refs->decode_channel &&
            refs->decode_channel(context, roots, refs->channel_context, type, &node, value);
    }
    if (*type == 'r' || *type == 'h')
    {
        return refs && (*type == 'r' || refs->graph) &&
            fvm_styxrpc_ref_decode(context, roots, refs, type, &node, value);
    }
    if (node.tag == STYXRPC_VALUE_NIL && !node.size && (*type == 's' || (*type == 'a' || *type == 'l')))
    {
        fvm_value_undefined(value);
        value->type = FVM_VALUE_NULL;
        return 1;
    }
    if (*type == 's')
    {
        return node.tag == STYXRPC_VALUE_STRING &&
            fvm_styxrpc_return_text(context, (const char *)node.payload, node.size, value) == FVM_OK &&
            fvm_gc_roots_add(roots, value) == FVM_OK;
    }
    if (*type == '(' || *type == 'a' || *type == 'l')
    {
        return (*type != 'l' || node.size) &&
            node.tag == (*type == '(' ? STYXRPC_VALUE_TUPLE : STYXRPC_VALUE_ARRAY) &&
            decode_compound(context, roots, refs, type, node, value) && fvm_gc_roots_add(roots, value) == FVM_OK;
    }
    if (*type == 'f')
    {
        double number;
        if (styxrpc_value_get_real(bytes, used, &number) != STYXRPC_OK)
        {
            return 0;
        }
        fvm_value_number(value, number);
        return 1;
    }
    if (styxrpc_value_get_integer(integer_tag(*type), bytes, used, &integer) != STYXRPC_OK)
    {
        return 0;
    }
    if (*type == 'b')
    {
        return fvm_styxrpc_return_big(context, integer, value) == FVM_OK &&
            (value->type != FVM_VALUE_STRING || fvm_gc_roots_add(roots, value) == FVM_OK);
    }
    fvm_value_number(value, (double)integer);
    return 1;
}


/* [GC:] EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_values_decode(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *shape,
    const FVM_SRecord *record, FVM_Value *values, unsigned capacity, unsigned *count)
{
    const char *type = shape + 1;
    FVM_SRecord cursor = *record;
    unsigned i = 0;
    while (*type != ')')
    {
        if (i == capacity || !decode_value(context, roots, refs, type, &cursor, values + i))
        {
            return 0;
        }
        ++i;
        type = fvm_styxrpc_type_end(type);
    }
    *count = i;
    return cursor.size == 0;
}
