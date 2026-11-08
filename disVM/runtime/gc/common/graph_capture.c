/* Copyright (c) 2026 bhgv
 *
 * EN: Guarded field capture and outside-reference classification for cyclic GC.
 * RU: Чтение полей под блокировками и поиск внешних ссылок для сборки циклов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "graph_snapshot.h"
#include "disvm/refcount.h"
#include "../../root_registry.h"
#include "../../runtime_gc.h"
#include <limits.h>
#include <string.h>

extern int dvm_runtime_channel_is_quiet(Channel *channel); /* Native channel state check. */


/* EN: Read field cell of source index under access, or caller-owned exclusive admission. Append it to group.
 * RU: Прочитать поле cell узла index под access или эксклюзивным допуском и добавить ребро в group. */
static int record_group_edge(RecordGroup *group, unsigned int index, void *cell,
    DVM_RangeAccess *access)
{
    if (!dvm_rc_reserve_record_fields(group, 1))
    {
        return 0;
    }
    void *edge;
    if (access)
    {
        DVM_CellResult result = dvm_range_access_extend(access, cell, sizeof(edge));
        if (result != DVM_CELL_ACQUIRED)
        {
            group->busy = result == DVM_CELL_BUSY;
            return 0;
        }
    }
    memcpy(&edge, cell, sizeof(edge));
    if (!edge)
    {
        return 0;
    }
    group->cells[group->edges_used] = cell;
    group->sources[group->edges_used] = index;
    if (edge == H)
    {
        group->targets[group->edges_used++] = UINT_MAX;
        return 1;
    }
    unsigned int target = dvm_rc_record_group_index(group, edge);
    group->incoming[target]++;
    group->targets[group->edges_used++] = target;
    return 1;
}


/* EN: Count mapped fields in type for elements repeated values, within available slots. Store the count in mapped.
 * RU: Проверить, помещаются ли ссылочные поля elements элементов типа type в available мест. Число полей вернуть в mapped. */
static int repeated_record_fields_fit(const Type *type, size_t elements,
    size_t available, size_t *mapped)
{
    size_t limit = available / elements;
    *mapped = 0;
    for (int byte = 0; byte < type->np; byte++)
    {
        *mapped += __builtin_popcount((unsigned char)type->map[byte]);
        if (*mapped > limit)
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Capture node index in group. start_roots is the expected seed root count; access guards fields when supplied.
 * RU: Прочитать узел index в group. start_roots задаёт ожидаемое число корней начального узла, access защищает поля. */
static int capture_record_source(RecordGroup *group, unsigned int index,
    unsigned long start_roots, DVM_RangeAccess *access)
{
    void *record = group->nodes[index];
    Heap *heap = D2H(record);
    Type *type = heap->t;
    unsigned char *data = record;
    size_t elements = 1;
    if (type == &Tarray)
    {
        Array *array = record;
        if (!array->root || !array->t)
        {
            return 0;
        }
        if (array->root != H)
        {
            group->has_views = 1;
            type = &Tptr;
            data = (unsigned char *)&array->root;
        }
        else
        {
            if (array->len < 0 ||
                array->data != (unsigned char *)array + sizeof(*array))
            {
                group->live[index] = 1;
                return 1;
            }
            type = array->t;
            elements = type->np ? array->len : 0;
            data = array->data;
        }
    }
    if (heap->t == &Tlist)
    {
        List *list = record;
        if (!record_group_edge(group, index, &list->tail, access))
        {
            return 0;
        }
        type = list->t;
        data = (unsigned char *)list->data;
        elements = type ? 1 : 0;
    }
    if (heap->t == &Tchannel)
    {
        Channel *channel = record;
        if (!dvm_runtime_channel_is_quiet(channel))
        {
            group->live[index] = 1;
            return 1;
        }
        type = &Tptr;
        data = (unsigned char *)&channel->buf;
    }
    if (heap->t != &Tlist && (!type || type->size < (int)sizeof(void *) ||
        (heap->t != &Tarray && heap->t != &Tchannel && type->free != freeheap)))
    {
        group->live[index] = 1;
        return 1;
    }
    if (dvm_runtime_root_count(heap) != (index == 0 ? start_roots : 0))
    {
        group->live[index] = 1;
    }
    if (elements > 1)
    {
        size_t mapped;
        size_t available = group->growing ? RECORD_SCRATCH_BYTES : group->field_limit - group->edges_used;
        if (!repeated_record_fields_fit(type, elements, available, &mapped))
        {
            group->oversized = 1;
            return 0;
        }
        if (!mapped)
        {
            return 1;
        }
        if (!dvm_rc_reserve_record_fields(group, elements * mapped))
        {
            return 0;
        }
    }
    for (size_t element = 0; element < elements; element++)
    {
        for (int byte = 0; byte < type->np; byte++)
        {
            for (unsigned int bit = 0; bit < 8; bit++)
            {
                if (!(type->map[byte] & (0x80U >> bit)))
                {
                    continue;
                }
                size_t slot = (size_t)byte * 8 + bit;
                void *edge;
                if (slot >= (size_t)type->size / sizeof(edge))
                {
                    return 0;
                }
                void *cell = data + element * type->size + slot * sizeof(edge);
                if (!record_group_edge(group, index, cell, access))
                {
                    return 0;
                }
            }
        }
    }
    return 1;
}


/* EN: Mark nodes in group reachable from outside. shared permits dropping an unclosed candidate as live.
 * RU: Отметить внешне достижимые узлы group. В режиме shared незамкнутого кандидата можно считать живым. */
static int classify_record_group(RecordGroup *group, int shared)
{
    if (!group->incoming[0])
    {
        if (!shared)
        {
            return 0;
        }
        group->live[0] = 1;
    }
    /* Discovery is complete; reuse its index as two dense work arrays. */
    unsigned int *ready = group->index;
    unsigned int *starts = ready + group->nodes_used;
    unsigned int edge = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        Heap *heap = D2H(group->nodes[index]);
        unsigned long expected = group->incoming[index] + (index == 0);
        unsigned long actual = dvm_refcount_load_u(&heap->ref);
        if (actual < expected)
        {
            return 0;
        }
        group->live[index] |= actual > expected;
        if (group->live[index])
        {
            ready[group->live_count++] = index;
        }
        starts[index] = edge;
        while (edge < group->edges_used && group->sources[edge] == index)
        {
            edge++;
        }
    }
    starts[group->nodes_used] = edge;

    /* Edges are captured in source order. Visit each live node's outgoing
     * range once, even when reachability runs against discovery order. */
    for (unsigned int head = 0; head < group->live_count && !group->live[0]; head++)
    {
        unsigned int source = ready[head];
        for (edge = starts[source]; edge < starts[source + 1]; edge++)
        {
            unsigned int target = group->targets[edge];
            if (target != UINT_MAX && !group->live[target])
            {
                group->live[target] = 1;
                ready[group->live_count++] = target;
            }
        }
    }
    return 1;
}


/* EN: Capture from rooted start into group. start_roots gives its expected roots; access is borrowed and remains held.
 * RU: Построить group от удерживаемого start. start_roots задаёт ожидаемое число корней, access остаётся захваченным. */
int dvm_rc_queued_record_group(void *start, unsigned long start_roots, RecordGroup *group,
    DVM_RangeAccess *access)
{
    if (!dvm_runtime_idle_cycles || gchalt || heapmonitor)
    {
        return 0;
    }
    (void)dvm_rc_record_group_index(group, start);
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        if (!capture_record_source(group, index, start_roots, access))
        {
            return 0;
        }
    }
    return classify_record_group(group, access != NULL);
}
