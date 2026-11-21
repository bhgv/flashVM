/* Copyright (c) 2026 bhgv
 * EN: Validate compiled call shapes against the exact module export table.
 * RU: Проверка скомпилированных схем по фактической таблице экспортов модуля.
 */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "module_shapes.h"
#include <stdlib.h>
#include <string.h>

#define SHAPE_TABLE_LIMIT 65536U /* Metadata bytes after the eight-byte marker. */

typedef struct DVM_ExportShapes
{
    size_t size; /* Validated encoded table size. */
    unsigned char bytes[]; /* Immutable records through the end marker. */
} DVM_ExportShapes;


/* EN: Read an unsigned 32-bit number from four available network-order bytes.
 * RU: Прочитать беззнаковое 32-битное число из четырёх байтов сетевого порядка. */
static uint32_t word(const unsigned char *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
        ((uint32_t)bytes[2] << 8) | bytes[3];
}


/* EN: Copy one record of known length, with exact text lengths and grammar.
 * RU: Скопировать одну запись проверенной длины и проверить грамматику текста. */
static int copy_shape(const unsigned char *entry, size_t available, StyxCallShape *shape, size_t *record)
{
    if (available < 10 || entry[8] > STYX_SHAPE_MAX || entry[9] > STYX_SHAPE_MAX)
    {
        return 0;
    }
    size_t a = entry[8], r = entry[9], size = 10 + a + r;
    if (size > available || memchr(entry + 10, 0, a + r))
    {
        return 0;
    }
    StyxCallShape decoded = {{0}, {0}};
    memcpy(decoded.arguments, entry + 10, a);
    memcpy(decoded.result, entry + 10 + a, r);
    if (!styxshapevalid(&decoded))
    {
        return 0;
    }
    *shape = decoded;
    *record = size;
    return 1;
}


/* EN/RU: See module_shapes.h. / См. module_shapes.h. */
int dvm_module_read_shapes(Module *module, const unsigned char *start, const unsigned char *end)
{
    const unsigned char *path_end = memchr(start, 0, (size_t)(end - start));
    if (!path_end || end - path_end - 1 < 8 || memcmp(path_end + 1, "STYXCS1!", 8))
    {
        return 1;
    }
    const unsigned char *bytes = path_end + 9;
    size_t size = (size_t)(end - bytes);
    if (size > SHAPE_TABLE_LIMIT)
    {
        return 0;
    }
    size_t exports = 0;
    while (module->ext && module->ext[exports].name)
    {
        exports++;
    }
    size_t offset = 0;
    size_t after = 0;
    while (size - offset >= 4)
    {
        const unsigned char *entry = bytes + offset;
        uint32_t index = word(entry);
        if (index == UINT32_MAX)
        {
            if (size - offset != 4)
            {
                return 0;
            }
            DVM_ExportShapes *table = malloc(sizeof(*table) + size);
            if (!table)
            {
                return 0;
            }
            table->size = size;
            memcpy(table->bytes, bytes, size);
            module->export_shapes = table;
            return 1;
        }
        StyxCallShape checked;
        size_t extent;
        if (index < after || index >= exports || !module->ext[index].frame || !module->ext[index].u.pc ||
            !copy_shape(entry, size - offset, &checked, &extent) || word(entry + 4) != (uint32_t)module->ext[index].sig)
        {
            return 0;
        }
        after = (size_t)index + 1;
        offset += extent;
    }
    return 0;
}


/* EN/RU: See module_shapes.h. / См. module_shapes.h. */
int dvm_module_export_shape(const Module *module, size_t index, StyxCallShape *shape)
{
    const DVM_ExportShapes *table = module->export_shapes;
    if (!table)
    {
        return 0;
    }
    for (size_t offset = 0; table->size - offset >= 4;)
    {
        const unsigned char *entry = table->bytes + offset;
        uint32_t current = word(entry);
        if (current > index || current == UINT32_MAX)
        {
            break;
        }
        if (current == index)
        {
            size_t extent;
            return copy_shape(entry, table->size - offset, shape, &extent);
        }
        offset += 10U + entry[8] + entry[9];
    }
    return 0;
}
