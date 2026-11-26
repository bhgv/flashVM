/* Copyright (c) 2026 bhgv
 * EN: Common admission for counted cell values.
 * RU: Общая проверка кадров значений с длиной для ячеек.
 */
#include "styxrpc/named_cell.h"
#include <string.h>


/* EN/RU: See named_cell.h. / См. named_cell.h. */
int styxrpc_cell_scalar(const void *frame, size_t size)
{
    if (!frame || !size || size > STYXRPC_NAMED_MAX)
    {
        return 0;
    }
    const unsigned char *bytes = frame;
    switch (bytes[0])
    {
    case STYXRPC_NAMED_NUMBER:
        return size == 1 + STYXRPC_NAMED_NUMBER_BYTES;
    case STYXRPC_NAMED_TEXT:
        return 1;
    case STYXRPC_NAMED_BOOLEAN:
        return size == 2 && bytes[1] <= 1;
    case STYXRPC_NAMED_NULL:
    case STYXRPC_NAMED_UNDEFINED:
        return size == 1;
    default:
        return 0;
    }
}


#define CELL_GRAPH_HEADER (1U + 16U) /* Named tag plus the FVMG header. */
#define CELL_GRAPH_VERSION_OFFSET 8U /* Index of the FVMG version after the named tag. */
#define CELL_GRAPH_MIN_VERSION 1U    /* Earliest snapshot envelope recognized by FlashVM. */
#define CELL_GRAPH_MAX_VERSION 9U    /* Latest snapshot envelope recognized by FlashVM. */
#define CELL_GRAPH_SIZE_OFFSET 9U    /* Big-endian graph body size in the FVMG header. */
#define CELL_GRAPH_CRC_OFFSET 13U    /* Big-endian IEEE CRC-32 of the graph body. */
#define CELL_GRAPH_CRC_POLY UINT32_C(0xedb88320) /* Reflected IEEE CRC-32 polynomial. */


/* EN: Read a big-endian 32-bit field. bytes must point at four readable bytes.
 * RU: Прочитать 32-битное поле старшим байтом вперёд из четырёх доступных bytes. */
static uint32_t graph_word(const unsigned char *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
        ((uint32_t)bytes[2] << 8) | bytes[3];
}


/* EN: Return the IEEE CRC-32 of the size borrowed bytes in the snapshot body.
 * RU: Вычислить IEEE CRC-32 тела снимка из size заимствованных байтов. */
static uint32_t graph_crc(const unsigned char *bytes, size_t size)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < size; ++i)
    {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1) ^ ((0U - (crc & 1U)) & CELL_GRAPH_CRC_POLY);
        }
    }
    return crc ^ UINT32_MAX;
}


/* EN/RU: See named_cell.h. / См. named_cell.h. */
int styxrpc_cell_value(const void *frame, size_t size)
{
    if (styxrpc_cell_scalar(frame, size))
    {
        return 1;
    }
    const unsigned char *bytes = frame;
    return bytes && size > CELL_GRAPH_HEADER && size <= STYXRPC_NAMED_MAX &&
        bytes[0] == STYXRPC_NAMED_VALUE && !memcmp(bytes + 1, "FVMG\0\0\0", 7) &&
        bytes[CELL_GRAPH_VERSION_OFFSET] >= CELL_GRAPH_MIN_VERSION &&
        bytes[CELL_GRAPH_VERSION_OFFSET] <= CELL_GRAPH_MAX_VERSION &&
        graph_word(bytes + CELL_GRAPH_SIZE_OFFSET) == size - CELL_GRAPH_HEADER &&
        graph_word(bytes + CELL_GRAPH_CRC_OFFSET) == graph_crc(bytes + CELL_GRAPH_HEADER, size - CELL_GRAPH_HEADER);
}
