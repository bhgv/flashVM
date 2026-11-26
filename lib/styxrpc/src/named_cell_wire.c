/* Copyright (c) 2026 bhgv
 * EN: Common admission for the counted scalar cell profile.
 * RU: Общая проверка скалярных кадров с длиной для ячеек.
 */
#include "styxrpc/named_cell.h"


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
