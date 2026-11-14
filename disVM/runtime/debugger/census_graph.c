/* Copyright (c) 2026 bhgv
 * EN: Read GC maps under one temporary incoming-edge guard set; never mark/retain.
 * RU: Читать GC maps под временным набором incoming-edge guards; без mark/retain.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "managed_owner.h"
#include "internal.h"
#include "census.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

extern Pool *heapmem; /* All observed objects remain held by their original guarded owners. */


/* [GC:] EN/RU: Extend read guards without waiting; contention is an explicit retry result.
 * Расширить read guards без ожидания; contention — явный retry result. */
static int guard(DebugCensus *census, void *address, size_t size)
{
    DVM_CellResult result = dvm_range_access_extend(&census->access, address, size);
    if (result != DVM_CELL_ACQUIRED)
    {
        census->flags |= result == DVM_CELL_BUSY ? 4 : 8;
    }
    return result == DVM_CELL_ACQUIRED;
}


/* [GC:] EN: Capture a rooted pointer under its source-cell guard. The full incoming
 * chain remains locked until query end, so identity cannot be freed/reused mid-scan.
 * RU: Снять rooted pointer под source-cell guard. Вся incoming chain остаётся
 * закрыта до конца query: identity не может быть freed/reused во время scan. */
int dvm_debug_census_field(DebugCensus *census, void *cell, int root)
{
    void *value;
    unsigned int slot;
    Type *type;
    unsigned int kind;
    ulong bytes;
    if (census->fields == CENSUS_FIELDS)
    {
        census->flags |= 2;
        return 0;
    }
    if (!guard(census, cell, sizeof(value)))
    {
        return 0;
    }
    census->fields++;
    memcpy(&value, cell, sizeof(value));
    if (!value || value == H)
    {
        return 1;
    }
    census->roots += root != 0;
    slot = (unsigned int)(((uintptr_t)value >> 4) ^ ((uintptr_t)value >> 17)) & 511U;
    while (census->index[slot])
    {
        if (census->nodes[census->index[slot] - 1] == value)
        {
            return 1;
        }
        slot = (slot + 1) & 511U;
    }
    if (census->count == CENSUS_NODES)
    {
        census->flags |= 2;
        return 0;
    }
    type = D2H(value)->t;
    bytes = poolmsize(heapmem, D2H(value));
    if (bytes < sizeof(Heap))
    {
        census->flags |= 8;
        return 0;
    }
    kind = type == &Tarray                               ? 0
           : type == &Tstring                            ? 1
           : type == &Tlist                              ? 2
           : type == &Tchannel                           ? 3
           : type == &Tmodlink                           ? 4
           : type && type != H && type->mark == markheap ? 5
                                                         : 6;
    census->nodes[census->count] = value;
    census->index[slot] = ++census->count;
    census->bytes += bytes;
    census->kinds[kind]++;
    census->kind_bytes[kind] += bytes;
    return 1;
}


/* [GC:] EN/RU: Bound descriptor/map/data geometry and charge every descriptor byte.
 * Ограничить descriptor/map/data geometry и учесть каждый descriptor byte. */
int dvm_debug_census_map(DebugCensus *census, Type *type, unsigned char *data, size_t extent, int root)
{
    int byte;
    if (!type || type == H || type->size < 0 || type->np < 0 || (size_t)type->size > extent ||
        (size_t)type->np > extent / (8 * sizeof(void *)) + (extent % (8 * sizeof(void *)) != 0))
    {
        census->flags |= 8;
        return 0;
    }
    for (byte = 0; byte < type->np; byte++)
    {
        unsigned int bit;
        if (census->maps++ == CENSUS_MAP_BYTES)
        {
            census->flags |= 2;
            return 0;
        }
        for (bit = 0; bit < 8; bit++)
        {
            size_t slot = (size_t)byte * 8 + bit;
            if (!(type->map[byte] & (0x80U >> bit)))
            {
                continue;
            }
            if (slot >= (size_t)type->size / sizeof(void *))
            {
                census->flags |= 8;
                return 0;
            }
            if (!dvm_debug_census_field(census, data + slot * sizeof(void *), root))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* [GC:] EN: Follow GC-equivalent special edges or immutable mapped payloads.
 * Custom callbacks are never executed; unsupported kinds mark the census partial.
 * RU: Следовать GC-equivalent special edges либо immutable mapped payloads.
 * Custom callbacks не исполняются; unsupported kinds делают census partial. */
static int node(DebugCensus *census, void *value)
{
    Type *type = D2H(value)->t;
    size_t extent = poolmsize(heapmem, D2H(value)) - sizeof(Heap);
    if (type == &Tarray)
    {
        Array copy;
        size_t index;
        if (extent < sizeof(copy) || !guard(census, value, sizeof(copy)))
        {
            return 0;
        }
        memcpy(&copy, value, sizeof(copy));
        if (!dvm_debug_census_field(census, &((Array *)value)->root, 0))
        {
            return 0;
        }
        if (copy.root != H && copy.root != NULL)
        {
            return 1;
        }
        if (!copy.t || copy.t == H || copy.t->size < 0 || copy.len < 0 || copy.data != (uchar *)value + sizeof(Array) ||
            (copy.t->size && (size_t)copy.len > (extent - sizeof(Array)) / (size_t)copy.t->size))
        {
            census->flags |= 8;
            return 0;
        }
        if (!copy.t->np)
        {
            return 1;
        }
        for (index = 0; index < (size_t)copy.len; index++)
        {
            if (!dvm_debug_census_map(census, copy.t, copy.data + index * copy.t->size, copy.t->size, 0))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type == &Tlist)
    {
        List *list = value;
        if (extent < offsetof(List, data) || !guard(census, value, offsetof(List, data)) ||
            !dvm_debug_census_field(census, &list->tail, 0))
        {
            return 0;
        }
        return !list->t || dvm_debug_census_map(census, list->t, (uchar *)list->data, extent - offsetof(List, data), 0);
    }
    if (type == &Tmodlink)
    {
        Modlink *link = value;
        if (extent < offsetof(Modlink, links) || !dvm_debug_census_field(census, &link->MP, 0))
        {
            return 0;
        }
        return !link->m || link->m == H || dvm_debug_census_field(census, &link->m->origmp, 0);
    }
    if (type == &Tstring || (type && type != H && type->mark == noptrs))
    {
        return 1;
    }
    if (type && type != H && type->mark == markheap)
    {
        return dvm_debug_census_map(census, type, value, extent, 0);
    }
    census->flags |= 16;
    return 1;
}


/* [GC:] EN/RU: Process each discovered identity once; no recursive C stack growth.
 * Обработать каждую identity один раз; без recursive C stack growth. */
void dvm_debug_census_graph(DebugCensus *census)
{
    unsigned int index;
    for (index = 0; index < census->count; index++)
    {
        if (!node(census, census->nodes[index]))
        {
            if (!census->flags)
            {
                census->flags = 8;
            }
            return;
        }
    }
}
