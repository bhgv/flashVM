/* Copyright (c) 2026 bhgv
 * EN: Translate managed by-value records without retaining wire pointers in FlashVM.
 * RU: Перевод управляемых записей по значению без указателей на транспорт в FlashVM.
 */
#include "managed_values.h"
#include "flashvm/execution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FVM_STYXRPC_MAX_DEPTH 6U /* Bound schema recursion as in the Dis codec. */
#define FVM_STYXRPC_SAFE_BIG 9007199254740991.0 /* Largest uniformly exact AS integer range. */
#define FVM_STYXRPC_INDEX_BYTES 11U /* Decimal index plus NUL on 32-bit unsigned. */


/* EN: Return the end of one supported type or NULL, within the terminated schema.
 * RU: Вернуть конец одного допустимого типа или NULL в ограниченной схеме. */
static const char *type_end(const char *type, unsigned depth)
{
    const char *next;
    if (depth >= FVM_STYXRPC_MAX_DEPTH || !*type)
    {
        return NULL;
    }
    switch (*type)
    {
    case 'i': case 'b': case 'y': case 's': case 'f': case 'h':
        return type + 1;
    case 'a': case 'c':
        next = type[1] == '[' ? type_end(type + 2, depth + 1) : NULL;
        return next && *next == ']' ? next + 1 : NULL;
    case 'r':
        if (type[1] != '[')
        {
            return NULL;
        }
        next = type + 2;
        if (*next == ']')
        {
            return NULL;
        }
        while (next && *next && *next != ']')
        {
            next = type_end(next, depth + 1);
        }
        return next && *next == ']' ? next + 1 : NULL;
    case '(':
        next = type + 1;
        while (next && *next && *next != ')')
        {
            next = type_end(next, depth + 1);
        }
        return next && *next == ')' ? next + 1 : NULL;
    default:
        return NULL;
    }
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
const char *fvm_styxrpc_type_end(const char *type)
{
    return type_end(type, 0);
}


/* EN: Match the ordinary managed ref rules across the checked argument list.
 * RU: Проверить условия обычного профиля ref для списка аргументов. */
static int ref_slots(const char *type, unsigned *slots)
{
    const char *end = fvm_styxrpc_type_end(type);
    const char *child;
    if (*type == 'r')
    {
        unsigned fields = 0;
        for (child = type + 2; *child != ']'; child = fvm_styxrpc_type_end(child))
        {
            if (++fields > FVM_STYXRPC_REF_FIELDS)
            {
                return 0;
            }
        }
        return ++*slots <= FVM_STYXRPC_REF_SLOTS && !memchr(type + 1, 'r', (size_t)(end - type - 1));
    }
    if (*type == 'a')
    {
        return !memchr(type, 'r', (size_t)(end - type));
    }
    if (*type == '(')
    {
        for (child = type + 1; *child != ')'; child = fvm_styxrpc_type_end(child))
        {
            if (!ref_slots(child, slots))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN: Verify two complete type lists and zero or one result.
 * RU: Проверить два полных списка типов и пустой либо одиночный результат. */
static int schema_supported(const StyxRpcSchema *schema)
{
    const char *args;
    const char *result;
    if (!schema || !memchr(schema->arguments, 0, sizeof(schema->arguments)) ||
        !memchr(schema->result, 0, sizeof(schema->result)) ||
        schema->arguments[0] != '(' || schema->result[0] != '(')
    {
        return 0;
    }
    args = type_end(schema->arguments, 0);
    result = schema->result[1] == ')' ? schema->result + 1 : type_end(schema->result + 1, 1);
    return args && !*args && result && *result == ')' && !result[1];
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_values_supported(const StyxRpcSchema *schema)
{
    unsigned slots = 0;
    return schema_supported(schema) && !strpbrk(schema->result, "rhc") && !strpbrk(schema->arguments, "hc") &&
        ref_slots(schema->arguments, &slots);
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_graph_supported(const StyxRpcSchema *schema)
{
    return schema_supported(schema) && !strchr(schema->arguments, 'c') && !strchr(schema->result, 'c') &&
        (strpbrk(schema->arguments, "rh") || strpbrk(schema->result, "rh"));
}


/* EN: Check that channel elements use only by-value types within their exact token.
 * RU: Проверить, что элементы каждого канала используют только типы по значению. */
int fvm_styxrpc_channel_value_elements(const char *type)
{
    for (type = strchr(type, 'c'); type; type = strchr(type, 'c'))
    {
        const char *end = type_end(type, 0);
        if (!end || memchr(type + 2, 'c', (size_t)(end - type - 3)))
        {
            return 0;
        }
        type = end;
    }
    return 1;
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_channel_signature(const StyxRpcSchema *schema)
{
    return schema_supported(schema) && !strpbrk(schema->arguments, "rh") &&
        !strpbrk(schema->result, "rh") && (strchr(schema->arguments, 'c') || strchr(schema->result, 'c'));
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_channel_supported(const StyxRpcSchema *schema)
{
    return fvm_styxrpc_channel_signature(schema) && !strchr(schema->arguments, 'c');
}


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
    if (value->type != FVM_VALUE_NUMBER || !(value->number >= low && value->number <= high))
    {
        return 0;
    }
    *result = (int64_t)value->number;
    return (double)*result == value->number;
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
    const char *child = type + (*type == 'a' ? 2 : 1);
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
            !encode_value(context, roots, *type == 'a' && (!refs || !refs->graph) ? NULL : refs, child, &element, body + size, capacity - size, &placed))
        {
            break;
        }
        size += placed;
        if (*type == '(')
        {
            child = type_end(child, 0);
        }
    }
    encoded = i == (unsigned)count && (*type == 'a' || *child == ')') &&
        fvm_srecord_write(*type == 'a' ? STYXRPC_VALUE_ARRAY : STYXRPC_VALUE_TUPLE,
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
    if ((*type == 's' || *type == 'a') && value->type == FVM_VALUE_NULL)
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
    if (*type == '(' || *type == 'a')
    {
        return encode_compound(context, roots, refs, type, value, bytes, capacity, used);
    }
    return exact_integer(value, -FVM_STYXRPC_SAFE_BIG, FVM_STYXRPC_SAFE_BIG, &number) &&
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
        type = type_end(type, 0);
    }
    *used = size;
    return *type == ')';
}


/* [GC:] EN: Turn a counted UTF-8 record into a context-owned AS string.
 * RU: Преобразовать UTF-8 запись с длиной в строку AS во владении контекста. */
static int decode_string(FVM_Context *context, const FVM_SRecord *record, FVM_Value *value)
{
    FVM_Value borrowed;
    size_t count;
    unsigned short *units = malloc((record->size ? record->size : 1) * sizeof(*units));
    int ok = units && fvm_value_string_bytes(&borrowed, (const char *)record->payload, record->size) == FVM_OK &&
        fvm_value_get_utf16(&borrowed, units, record->size, &count) == FVM_OK &&
        fvm_value_string_utf16(context, units, count, value) == FVM_OK;
    free(units);
    return ok;
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
    const char *shape = type + (*type == 'a' ? 2 : 1);
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
        if (*shape == ')' || !decode_value(context, roots, *type == 'a' && (!refs || !refs->graph) ? NULL : refs, shape, &record, items + i))
        {
            break;
        }
        if (*type == '(')
        {
            shape = type_end(shape, 0);
        }
    }
    ok = i == count && (*type == 'a' || *shape == ')') &&
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
    if (node.tag == STYXRPC_VALUE_NIL && !node.size && (*type == 's' || *type == 'a'))
    {
        fvm_value_undefined(value);
        value->type = FVM_VALUE_NULL;
        return 1;
    }
    if (*type == 's')
    {
        return node.tag == STYXRPC_VALUE_STRING && decode_string(context, &node, value) &&
            fvm_gc_roots_add(roots, value) == FVM_OK;
    }
    if (*type == '(' || *type == 'a')
    {
        return node.tag == (*type == 'a' ? STYXRPC_VALUE_ARRAY : STYXRPC_VALUE_TUPLE) &&
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
    if (styxrpc_value_get_integer(integer_tag(*type), bytes, used, &integer) != STYXRPC_OK ||
        integer < -(int64_t)FVM_STYXRPC_SAFE_BIG || integer > (int64_t)FVM_STYXRPC_SAFE_BIG)
    {
        return 0;
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
        type = type_end(type, 0);
    }
    *count = i;
    return cursor.size == 0;
}
