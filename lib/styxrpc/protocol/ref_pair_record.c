/* Copyright (c) 2026 bhgv
 * EN: Encode one identity table and bounded int/big snapshots for a two-ref call.
 * RU: Кодирование таблицы идентичности и снимков int/big для вызова с двумя ref.
 */
#include "ref_pair_record.h"
#include "scalar_verify.h"
#include "flashvm/srecord.h"

#define REFPAIR_MAP 1UL /* Ordered argument IDs. */
#define REFPAIR_RESULT 2UL /* Ordinary result preceding updates. */
#define REFPAIR_OBJECT 3UL /* Local object ID followed by its fields. */
#define REFPAIR_CELL_SIZE 12U /* Four int bytes followed by eight big bytes. */


/* EN: Write width low bytes of scalar in network order to borrowed bytes.
 * RU: Записать width младших байтов scalar в bytes в сетевом порядке. */
static void put_integer(unsigned char *bytes, size_t width, uint64_t scalar)
{
    while (width)
    {
        bytes[--width] = (unsigned char)scalar;
        scalar >>= 8;
    }
}


/* EN: Read one signed word or big from width network-order bytes.
 * RU: Прочитать знаковый int или big из width байтов в сетевом порядке. */
static int64_t get_integer(const unsigned char *bytes, size_t width)
{
    uint64_t bits = 0;
    for (size_t i = 0; i < width; i++)
    {
        bits = (bits << 8) | bytes[i];
    }
    uint64_t mask = width == 4 ? UINT32_MAX : UINT64_MAX;
    return (bytes[0] & 0x80) ? -1 - (int64_t)(mask - bits) : (int64_t)bits;
}


/* EN: Append one nested tag and data of length size to a fixed-capacity payload.
 * RU: Добавить вложенную запись tag с data длины size в payload фиксированного размера. */
static int append(unsigned char *payload, size_t *offset, unsigned long tag, const void *data, size_t size)
{
    size_t used;
    if (fvm_srecord_write(tag, data, size, payload + *offset, STYX_FUNCTION_FRAME_CAPACITY - *offset, &used) != FVM_S_OK)
    {
        return 0;
    }
    *offset += used;
    return 1;
}


/* EN: Write both typed fields of value into the twelve bytes provided.
 * RU: Записать оба типизированных поля value в выделенные двенадцать байтов. */
static void put_cell(unsigned char *bytes, const StyxTuple *value)
{
    put_integer(bytes, 4, (uint32_t)value->narrow);
    put_integer(bytes + 4, 8, (uint64_t)value->wide);
}


/* EN: Read a cell after its record size and ID have been validated.
 * RU: Прочитать ячейку после проверки размера записи и идентификатора. */
static StyxTuple get_cell(const unsigned char *bytes)
{
    return (StyxTuple){(int32_t)get_integer(bytes, 4), get_integer(bytes + 4, 8)};
}


/* EN/RU: See ref_pair_record.h. / См. ref_pair_record.h. */
int styxrefpairwrite(unsigned long tag, const StyxRefPair *value, void *bytes, size_t capacity, size_t *used)
{
    if (!value || !bytes || !used || value->count < 1 || value->count > 2 ||
        (tag != STYX_REF_PAIR_REQUEST_TAG && tag != STYX_REF_PAIR_REPLY_TAG))
    {
        return 0;
    }
    unsigned char payload[STYX_FUNCTION_FRAME_CAPACITY];
    unsigned char cell[1 + REFPAIR_CELL_SIZE];
    const unsigned char map[] = {0, (unsigned char)(value->count - 1)};
    size_t offset = 0;
    if (tag == STYX_REF_PAIR_REPLY_TAG)
    {
        put_cell(cell, &value->result);
        if (!append(payload, &offset, REFPAIR_RESULT, cell, REFPAIR_CELL_SIZE))
        {
            return 0;
        }
    }
    if (!append(payload, &offset, REFPAIR_MAP, map, sizeof(map)))
    {
        return 0;
    }
    for (size_t i = 0; i < value->count; i++)
    {
        cell[0] = (unsigned char)i;
        put_cell(cell + 1, &value->objects[i]);
        if (!append(payload, &offset, REFPAIR_OBJECT, cell, sizeof(cell)))
        {
            return 0;
        }
    }
    size_t limit = capacity < STYX_FUNCTION_FRAME_CAPACITY ? capacity : STYX_FUNCTION_FRAME_CAPACITY;
    return fvm_srecord_write(tag, payload, offset, bytes, limit, used) == FVM_S_OK;
}


/* EN: Consume one exact-size nested record, or keep cursor and length unchanged.
 * RU: Прочитать одну вложенную запись точного размера либо сохранить cursor и remaining. */
static const unsigned char *take(const unsigned char **cursor, size_t *remaining, unsigned long tag, size_t size)
{
    FVM_SRecord node;
    size_t used;
    if (fvm_srecord_read(*cursor, *remaining, STYX_FUNCTION_FRAME_CAPACITY, &node, &used) != FVM_S_OK ||
        node.tag != tag || node.size != size)
    {
        return NULL;
    }
    *cursor += used;
    *remaining -= used;
    return node.payload;
}


/* EN/RU: See ref_pair_record.h. / См. ref_pair_record.h. */
int styxrefpairread(unsigned long tag, const void *bytes, size_t size, StyxRefPair *value)
{
    FVM_SRecord outer;
    size_t used;
    if (!value || !bytes || size > STYX_FUNCTION_FRAME_CAPACITY ||
        (tag != STYX_REF_PAIR_REQUEST_TAG && tag != STYX_REF_PAIR_REPLY_TAG) ||
        fvm_srecord_read(bytes, size, STYX_FUNCTION_FRAME_CAPACITY, &outer, &used) != FVM_S_OK ||
        used != size || outer.tag != tag)
    {
        return 0;
    }
    StyxRefPair staged = {0};
    const unsigned char *cursor = outer.payload;
    size_t remaining = outer.size;
    if (tag == STYX_REF_PAIR_REPLY_TAG)
    {
        const unsigned char *cell = take(&cursor, &remaining, REFPAIR_RESULT, REFPAIR_CELL_SIZE);
        if (!cell)
        {
            return 0;
        }
        staged.result = get_cell(cell);
    }
    const unsigned char *map = take(&cursor, &remaining, REFPAIR_MAP, 2);
    if (!map || map[0] != 0 || map[1] > 1)
    {
        return 0;
    }
    staged.count = 1 + map[1];
    for (size_t i = 0; i < staged.count; i++)
    {
        const unsigned char *cell = take(&cursor, &remaining, REFPAIR_OBJECT, 1 + REFPAIR_CELL_SIZE);
        if (!cell || cell[0] != i)
        {
            return 0;
        }
        staged.objects[i] = get_cell(cell + 1);
    }
    if (remaining)
    {
        return 0;
    }
    *value = staged;
    return 1;
}
