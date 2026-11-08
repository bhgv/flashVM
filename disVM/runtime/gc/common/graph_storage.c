/* Copyright (c) 2026 bhgv
 *
 * EN: Snapshot buffers and address indexing shared by Cycles and Duke GC.
 * RU: Общие буферы снимков и индекс адресов для Cycles и ДюкGC.
 */
#include "graph_snapshot.h"
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/* EN: Bind an empty snapshot to zeroed scratch owned by the caller.
 * RU: Связать пустой снимок с обнулённым буфером scratch вызывающей стороны. */
RecordGroup dvm_rc_empty_record_group(RecordGroupScratch *scratch)
{
    return (RecordGroup){.nodes = scratch->nodes, .incoming = scratch->incoming,
        .index = scratch->index, .cells = scratch->cells, .targets = scratch->targets,
        .sources = scratch->sources, .live = scratch->live, .field_limit = RECORD_EDGE_LIMIT};
}


/* EN: Count the dead-source fields of group in edges. A live-to-dead
 * captured edge makes the proposed collection unsafe and returns zero.
 * RU: Подсчитать в edges поля мёртвых узлов group. Ребро из живого узла
 * в мёртвый делает снимок непригодным и даёт нулевой результат. */
int dvm_rc_snapshot_edges(const RecordGroup *group, unsigned int *edges)
{
    *edges = group->edges_used;
    if (!group->live_count)
    {
        return 1;
    }
    *edges = 0;
    for (unsigned int index = 0; index < group->edges_used; index++)
    {
        unsigned int target = group->targets[index];
        if (!group->live[group->sources[index]])
        {
            (*edges)++;
        }
        else if (target != UINT_MAX && !group->live[target])
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Return bytes for the nodes of group and edges retained fields.
 * RU: Вернуть размер памяти узлов group и edges сохраняемых полей. */
size_t dvm_rc_record_group_bytes(const RecordGroup *group, unsigned int edges)
{
    size_t nodes = group->nodes_used;
    size_t dead_index = group->live_count ? nodes - group->live_count : 0;
    return sizeof(*group) + (nodes + edges) * sizeof(void *) +
        (nodes + 2 * edges + dead_index) * sizeof(unsigned int) + nodes * sizeof(unsigned char);
}


/* EN: Copy group with edges checked fields into one allocation. Return NULL
 * on failure. The caller keeps every referent live.
 * RU: Скопировать group с edges проверенными полями в один блок или вернуть NULL.
 * Вызывающая сторона удерживает все объекты. */
RecordGroup *dvm_rc_copy_record_group(const RecordGroup *group, unsigned int edges)
{
    size_t nodes = group->nodes_used;
    RecordGroup *copy = malloc(dvm_rc_record_group_bytes(group, edges));
    if (!copy)
    {
        return NULL;
    }
    *copy = *group;
    copy->nodes = (void **)(copy + 1);
    copy->cells = copy->nodes + nodes;
    copy->incoming = (unsigned int *)(copy->cells + edges);
    copy->targets = copy->incoming + nodes;
    copy->sources = copy->targets + edges;
    copy->dead_count = group->nodes_used - group->live_count;
    copy->dead_nodes = group->live_count ? copy->sources + edges : NULL;
    copy->live = (unsigned char *)(copy->sources + edges +
        (copy->dead_nodes ? copy->dead_count : 0));
    copy->index = NULL;
    copy->owned_scratch = NULL;
    copy->pinned_nodes = 0;
    copy->edges_used = edges;
    copy->omitted_live_fields = group->edges_used - edges;
    memcpy(copy->nodes, group->nodes, nodes * sizeof(*copy->nodes));
    memcpy(copy->incoming, group->incoming, nodes * sizeof(*copy->incoming));
    if (edges == group->edges_used)
    {
        memcpy(copy->cells, group->cells, edges * sizeof(*copy->cells));
        memcpy(copy->targets, group->targets, edges * sizeof(*copy->targets));
        memcpy(copy->sources, group->sources, edges * sizeof(*copy->sources));
    }
    else
    {
        unsigned int next = 0;
        for (unsigned int index = 0; index < group->edges_used; index++)
        {
            unsigned int source = group->sources[index];
            if (!group->live[source])
            {
                copy->cells[next] = group->cells[index];
                copy->targets[next] = group->targets[index];
                copy->sources[next] = source;
                next++;
            }
        }
    }
    memcpy(copy->live, group->live, nodes * sizeof(*copy->live));
    if (copy->dead_nodes)
    {
        unsigned int next = 0;
        for (unsigned int index = 0; index < copy->nodes_used; index++)
        {
            if (!copy->live[index])
            {
                copy->dead_nodes[next++] = index;
            }
        }
    }
    if (!dvm_range_plan_cells(&copy->field_plan, copy->cells, copy->edges_used))
    {
        free(copy);
        return NULL;
    }
    return copy;
}


/* EN: Find the first hash probe for value in the discovery index of group.
 * RU: Вычислить начальную позицию адреса value в индексе снимка group. */
unsigned int dvm_rc_record_index_slot(const RecordGroup *group, void *value)
{
    uintptr_t address = (uintptr_t)value / sizeof(void *);
    return (address ^ (address >> 7)) % (group->field_limit * 4);
}


/* EN: Find or insert the live payload value in group. The caller reserves space first.
 * RU: Найти или добавить живой объект value в group. Место резервирует вызывающий код. */
unsigned int dvm_rc_record_group_index(RecordGroup *group, void *value)
{
    unsigned int slots = group->field_limit * 4;
    unsigned int slot = dvm_rc_record_index_slot(group, value);
    while (group->index[slot])
    {
        unsigned int index = group->index[slot] - 1;
        if (group->nodes[index] == value)
        {
            return index;
        }
        slot = (slot + 1) % slots;
    }
    unsigned int index = group->nodes_used++;
    group->nodes[index] = value;
    group->index[slot] = index + 1;
    return index;
}


/* EN: Reserve fields additional slots in group. Mark an oversized capture on refusal.
 * RU: Зарезервировать fields дополнительных мест в group. При отказе отметить превышение размера. */
int dvm_rc_reserve_record_fields(RecordGroup *group, size_t fields)
{
    if (fields <= group->field_limit - group->edges_used ||
        (group->growing && fields <= RECORD_SCRATCH_BYTES &&
         dvm_rc_grow_record_group(group, group->edges_used + fields)))
    {
        return 1;
    }
    group->oversized = 1;
    return 0;
}
