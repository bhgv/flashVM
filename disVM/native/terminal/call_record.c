/* Copyright (c) 2026 bhgv
 * EN: Encode and validate recursive tuple shapes independent of the Dis frame layout.
 * RU: Кодирование и проверка вложенных кортежей независимо от раскладки кадра Dis.
 */
#include "call_record.h"
#include "tuple_record.h"
#include "scalar_record.h"
#include "flashvm/srecord.h"
#include <string.h>

#define CALL_INT 1UL /* Signed int leaf. */
#define CALL_BIG 2UL /* Signed big leaf. */
#define CALL_PID 3UL /* Session-local caller PID. */
#define CALL_TUPLE 4UL /* Nested tuple. */
#define CALL_REAL 5UL /* IEEE 754 binary64 bit pattern. */

const char styxcall_list_args[] = "(i(ib)b)"; /* Argument list: int, tuple, big. */
const char styxcall_list_result[] = "((i(ib)))"; /* One return value: (int,(int,big)). */


/* EN: Map one schema token to its nested record tag.
 * RU: Сопоставить токен схемы с тегом вложенной записи. */
static unsigned long node_tag(char type)
{
    return type == 'i' ? CALL_INT : type == 'b' ? CALL_BIG : type == 'f' ? CALL_REAL :
        type == 'p' ? CALL_PID : CALL_TUPLE;
}


/* EN: Check the allowed range of a scalar cell against its schema token.
 * RU: Проверить диапазон скалярной ячейки по токену схемы. */
static int scalar_valid(char type, int64_t value)
{
    return type == 'b' || type == 'f' || (type == 'i' && value >= INT32_MIN && value <= INT32_MAX) ||
        (type == 'p' && value > 0 && value <= UINT32_MAX);
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
