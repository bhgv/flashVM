/* Copyright (c) 2026 bhgv
 * EN: Encode and validate recursive tuple shapes independent of the Dis frame layout.
 * RU: Кодирование и проверка вложенных кортежей независимо от раскладки кадра Dis.
 */
#include "call_record.h"
#include "tuple_record.h"
#include "scalar_record.h"
#include "load_profile.h"
#include "flashvm/utf8.h"
#include "flashvm/srecord.h"
#include <string.h>

#define CALL_PID 3UL /* Session-local caller PID. */

const char styxcall_list_args[] = "(i(ib)b)"; /* Argument list: int, tuple, big. */
const char styxcall_list_result[] = "((i(ib)))"; /* One return value: (int,(int,big)). */


/* EN: Map one schema token to its nested record tag.
 * RU: Сопоставить токен схемы с тегом вложенной записи. */
static unsigned long node_tag(char type)
{
    return type == 'i' ? STYX_CALL_INT_TAG : type == 'b' ? STYX_CALL_BIG_TAG : type == 'f' ? STYX_CALL_REAL_TAG :
        type == 'p' ? CALL_PID : type == 'y' ? STYX_CALL_BYTE_TAG : STYX_CALL_TUPLE_TAG;
}


/* EN: Check the allowed range of a scalar cell against its schema token.
 * RU: Проверить диапазон скалярной ячейки по токену схемы. */
static int scalar_valid(char type, int64_t value)
{
    return type == 'b' || type == 'f' || (type == 'i' && value >= INT32_MIN && value <= INT32_MAX) ||
        (type == 'p' && value > 0 && value <= UINT32_MAX) ||
        (type == 'y' && value >= 0 && value <= UINT8_MAX);
}


/* EN: Consume one trusted type and write its owned temporary frame into bytes.
 * RU: Прочитать один доверенный тип и записать его временную запись в bytes.
 * Cursor tracks shape, cell tracks consumed inputs and depth bounds recursion.
 * Cursor задаёт схему, cell отслеживает поля, depth ограничивает вложенность. */
static int write_node(unsigned long tag, const char **cursor, unsigned depth,
    const StyxCallValues *values, size_t *cell, void *bytes, size_t capacity, size_t *used)
{
    char type = **cursor;
    if (!type || (type == '(' && depth >= STYX_CALL_DEPTH))
    {
        return 0;
    }
    ++*cursor;
    if (type == '(')
    {
        unsigned char payload[STYX_FUNCTION_FRAME_CAPACITY];
        size_t size = 0;
        while (**cursor != ')')
        {
            unsigned long child = node_tag(**cursor);
            size_t written;
            if (!write_node(child, cursor, depth + 1, values, cell, payload + size, sizeof(payload) - size, &written))
            {
                return 0;
            }
            size += written;
        }
        ++*cursor;
        return fvm_srecord_write(tag, payload, size, bytes, capacity, used) == FVM_S_OK;
    }
    if (*cell >= values->count)
    {
        return 0;
    }
    int64_t value = values->cells[(*cell)++];
    return scalar_valid(type, value) &&
        styxscalarwrite(tag, STYX_SCALAR_VALUE, value, bytes, capacity, used);
}


/* EN: Consume a single expected node and stage scalar cells in private values.
 * RU: Прочитать ожидаемый узел и сохранить скаляры во временном values.
 * used is the node extent; bytes remain borrowed. depth bounds recursion.
 * used получает размер узла, bytes заимствуется, depth ограничивает вложенность. */
static int read_node(unsigned long tag, const char **cursor, unsigned depth,
    const unsigned char *bytes, size_t size, size_t *used, StyxCallValues *values)
{
    FVM_SRecord record;
    char type = **cursor;
    if (!type || (type == '(' && depth >= STYX_CALL_DEPTH) ||
        fvm_srecord_read(bytes, size, STYX_FUNCTION_FRAME_CAPACITY, &record, used) != FVM_S_OK ||
        record.tag != tag)
    {
        return 0;
    }
    ++*cursor;
    if (type == '(')
    {
        size_t offset = 0;
        while (**cursor != ')')
        {
            unsigned long child = node_tag(**cursor);
            size_t length;
            if (!read_node(child, cursor, depth + 1, record.payload + offset, record.size - offset, &length, values))
            {
                return 0;
            }
            offset += length;
        }
        ++*cursor;
        return offset == record.size;
    }
    StyxScalarKind kind;
    int64_t value;
    if (values->count == STYX_CALL_VALUES ||
        !styxscalarread(tag, bytes, *used, &kind, &value) || kind != STYX_SCALAR_VALUE ||
        !scalar_valid(type, value))
    {
        return 0;
    }
    values->cells[values->count++] = value;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxcallrecordwrite(unsigned long tag, const char *shape, const StyxCallValues *values,
    void *bytes, size_t capacity, size_t *used)
{
    unsigned char frame[STYX_FUNCTION_FRAME_CAPACITY];
    size_t cell = 0, written;
    if (!shape || *shape != '(' || !values || values->count > STYX_CALL_VALUES || !bytes || !used ||
        !write_node(tag, &shape, 0, values, &cell, frame, sizeof(frame), &written) || *shape ||
        cell != values->count || written > capacity)
    {
        return 0;
    }
    memcpy(bytes, frame, written);
    *used = written;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxcallrecordread(unsigned long tag, const char *shape, const void *bytes, size_t size,
    StyxCallValues *values)
{
    size_t used;
    StyxCallValues decoded = {0};
    if (!shape || *shape != '(' || !bytes || !size || size > STYX_FUNCTION_FRAME_CAPACITY || !values ||
        !read_node(tag, &shape, 0, bytes, size, &used, &decoded) || *shape || used != size)
    {
        return 0;
    }
    *values = decoded;
    return 1;
}


/* EN: Align a bounded byte offset to a power-of-two NC64 field alignment.
 * RU: Выровнять ограниченное смещение по степени двойки для поля NC64. */
static size_t align_field(size_t offset, size_t alignment)
{
    return (offset + alignment - 1) & ~(alignment - 1);
}


/* EN: Append one field's leaf layout relative to its enclosing aggregate.
 * RU: Добавить раскладку полей одного значения относительно внешнего агрегата. */
static int scalar_field(const char **cursor, unsigned depth, int managed, StyxScalarLayout *layout,
    size_t *alignment)
{
    char type = **cursor;
    if (type == '(' && depth < (managed ? 7U : STYX_CALL_DEPTH))
    {
        ++*cursor;
        size_t base = layout->count;
        size_t size = 0;
        *alignment = 1;
        while (**cursor != ')')
        {
            size_t first = layout->count;
            size_t child_alignment;
            if (!scalar_field(cursor, depth + 1, managed, layout, &child_alignment))
            {
                return 0;
            }
            size = align_field(size, child_alignment);
            for (size_t i = first; i < layout->count; i++)
            {
                layout->offset[i] += size;
            }
            size += layout->size;
            if (child_alignment > *alignment)
            {
                *alignment = child_alignment;
            }
        }
        ++*cursor;
        if (depth && layout->count == base)
        {
            return 0;
        }
        layout->size = align_field(size, *alignment);
        return 1;
    }
    int pointer = managed && (type == 's' || type == 'a' || type == 'r');
    if ((!pointer && type != 'i' && type != 'b' && type != 'f' && type != 'y') ||
        layout->count == STYX_CALL_VALUES)
    {
        return 0;
    }
    if (pointer)
    {
        const char *end = styxshape_end(*cursor);
        if (!end)
        {
            return 0;
        }
        *cursor = end;
        layout->pointers |= 1U << layout->count;
    }
    else
    {
        ++*cursor;
    }
    *alignment = type == 'y' ? 1 : 8;
    layout->offset[layout->count] = 0;
    layout->width[layout->count++] = (unsigned char)*alignment;
    layout->size = *alignment;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxscalar_layout(const char *shape, StyxScalarLayout *layout)
{
    StyxScalarLayout decoded = {0};
    size_t alignment;
    if (!shape || *shape != '(' || !layout || !scalar_field(&shape, 0, 0, &decoded, &alignment) || *shape)
    {
        return 0;
    }
    *layout = decoded;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_layout(const char *shape, StyxScalarLayout *layout)
{
    StyxScalarLayout decoded = {0};
    size_t alignment;
    StyxCallShape schema = {.result = "()"};
    if (!shape || !layout)
    {
        return 0;
    }
    size_t length = strlen(shape);
    if (length > STYX_SHAPE_MAX)
    {
        return 0;
    }
    memcpy(schema.arguments, shape, length + 1);
    if (!styxshapevalid(&schema) || !scalar_field(&shape, 0, 1, &decoded, &alignment) || *shape)
    {
        return 0;
    }
    *layout = decoded;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
void styxscalar_unpack(const StyxScalarLayout *layout, const void *frame, StyxCallValues *values)
{
    const unsigned char *bytes = frame;
    *values = (StyxCallValues){.count = layout->count};
    for (size_t i = 0; i < layout->count; i++)
    {
        const unsigned char *field = bytes + layout->offset[i];
        if (layout->width[i] == 1)
        {
            values->cells[i] = *field;
        }
        else
        {
            memcpy(&values->cells[i], field, sizeof(values->cells[i]));
        }
    }
}


/* EN/RU: See call_record.h. / См. call_record.h. */
void styxscalar_pack(const StyxScalarLayout *layout, const StyxCallValues *values, void *frame)
{
    unsigned char *bytes = frame;
    memset(bytes, 0, layout->size);
    for (size_t i = 0; i < layout->count; i++)
    {
        unsigned char *field = bytes + layout->offset[i];
        if (layout->width[i] == 1)
        {
            *field = (unsigned char)values->cells[i];
        }
        else
        {
            memcpy(field, &values->cells[i], sizeof(values->cells[i]));
        }
    }
}


/* EN: Validate one tuple's exact arity or one array's homogeneous record sequence.
 * RU: Проверить точное число полей кортежа или однородную последовательность массива. */
static int managed_children(const char *shape, const unsigned char *bytes, size_t size, int array,
    unsigned depth);


/* EN: Traverse one nested value, checking tag, payload and bounds before advancing.
 * RU: Проверить тег, содержимое и границы вложенного значения до продвижения. */
static int managed_value(const char *shape, const unsigned char *bytes, size_t size, unsigned depth, size_t *used)
{
    FVM_SRecord value;
    if (depth == 6 || fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &value, used) != FVM_S_OK)
    {
        return 0;
    }
    const char type = *shape;
    if ((type == 's' || type == 'a') && value.tag == STYX_MANAGED_NIL_TAG)
    {
        return value.size == 0;
    }
    switch (type)
    {
    case 's':
        return value.tag == STYX_MANAGED_STRING_TAG &&
            fvm_utf8_validate((const char *)value.payload, value.size, NULL) == FVM_UTF8_OK;
    case 'a':
        return value.tag == STYX_MANAGED_ARRAY_TAG &&
            managed_children(shape + 2, value.payload, value.size, 1, depth + 1);
    case '(':
        return value.tag == STYX_CALL_TUPLE_TAG && managed_children(shape + 1, value.payload, value.size, 0, depth + 1);
    case 'i':
    case 'b':
    case 'f':
    case 'y':
    {
        StyxScalarKind kind;
        int64_t scalar;
        return styxscalarread(node_tag(type), bytes, *used, &kind, &scalar) &&
            kind == STYX_SCALAR_VALUE && scalar_valid(type, scalar);
    }
    default:
        return 0;
    }
}


/* EN: Scan borrowed child records once. Shape is already checked as a complete list.
 * RU: Один раз пройти заимствованные записи. Схема уже проверена целиком. */
static int managed_children(const char *shape, const unsigned char *bytes, size_t size, int array,
    unsigned depth)
{
    while (array ? size != 0 : *shape != ')')
    {
        size_t used;
        if (!managed_value(shape, bytes, size, depth, &used))
        {
            return 0;
        }
        if (!array)
        {
            shape = styxshape_end(shape);
        }
        bytes += used;
        size -= used;
    }
    return size == 0;
}


/* EN: Apply the compiled schema grammar and exclude reference identity from this codec.
 * RU: Проверить грамматику схемы Dis и исключить идентичность ref из кодека. */
int styxmanaged_payload_valid(const char *shape, const void *bytes, size_t size)
{
    StyxCallShape schema = {.result = "()"};
    if (!shape || (size && !bytes))
    {
        return 0;
    }
    size_t length = strlen(shape);
    if (length > STYX_SHAPE_MAX || strchr(shape, 'r'))
    {
        return 0;
    }
    memcpy(schema.arguments, shape, length + 1);
    return styxshapevalid(&schema) && managed_children(schema.arguments + 1, bytes, size, 0, 0);
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_list_read(unsigned long tag, const char *shape, const void *bytes, size_t size,
    FVM_SRecord *list)
{
    FVM_SRecord decoded;
    size_t used;
    if (!list || size > STYX_MANAGED_LIST_LIMIT ||
        fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &decoded, &used) != FVM_S_OK ||
        used != size || decoded.tag != tag || !styxmanaged_payload_valid(shape, decoded.payload, decoded.size))
    {
        return 0;
    }
    *list = decoded;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_list_write(unsigned long tag, const char *shape, const void *payload, size_t size,
    void *bytes, size_t capacity, size_t *used)
{
    if (capacity > STYX_MANAGED_LIST_LIMIT)
    {
        capacity = STYX_MANAGED_LIST_LIMIT;
    }
    return size <= capacity && styxmanaged_payload_valid(shape, payload, size) &&
        fvm_srecord_write(tag, payload, size, bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxcall_reply_write(unsigned long tag, const void *result, size_t result_size,
    const void *updates, size_t updates_size, void *bytes, size_t capacity, size_t *used)
{
    FVM_SRecord item;
    size_t length;
    if (capacity > STYX_MANAGED_LIST_LIMIT)
    {
        capacity = STYX_MANAGED_LIST_LIMIT;
    }
    return result_size <= capacity && updates_size <= capacity - result_size &&
        fvm_srecord_read(result, result_size, capacity, &item, &length) == FVM_S_OK && length == result_size &&
        fvm_srecord_read(updates, updates_size, capacity, &item, &length) == FVM_S_OK && length == updates_size &&
        fvm_srecord_write_pair(tag, result, result_size, updates, updates_size, bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxcall_reply_read(unsigned long tag, const void *bytes, size_t size, StyxReplyField fields[2])
{
    FVM_SRecord outer, item;
    size_t used, first, second;
    if (!fields || size > STYX_MANAGED_LIST_LIMIT ||
        fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &outer, &used) != FVM_S_OK ||
        used != size || outer.tag != tag ||
        fvm_srecord_read(outer.payload, outer.size, STYX_MANAGED_LIST_LIMIT, &item, &first) != FVM_S_OK ||
        fvm_srecord_read(outer.payload + first, outer.size - first, STYX_MANAGED_LIST_LIMIT, &item, &second) != FVM_S_OK ||
        second != outer.size - first)
    {
        return 0;
    }
    fields[0] = (StyxReplyField){outer.payload, first};
    fields[1] = (StyxReplyField){outer.payload + first, second};
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_is_nil(const void *bytes, size_t size)
{
    FVM_SRecord record;
    size_t used;
    return fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &record, &used) == FVM_S_OK &&
        used == size && record.tag == STYX_MANAGED_NIL_TAG && !record.size;
}
