/* Copyright (c) 2026 bhgv
 *
 * EN: Snapshot buffers and address indexing shared by Cycles and Duke GC.
 * RU: Общие буферы снимков и индекс адресов для Cycles и ДюкGC.
 */
#include "graph_snapshot.h"
#include <stdint.h>
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


/* EN: Return the bytes needed to copy captured nodes and fields in group.
 * RU: Вернуть размер копии прочитанных узлов и полей снимка group. */
size_t dvm_rc_record_group_bytes(const RecordGroup *group)
{
    size_t nodes = group->nodes_used;
    size_t edges = group->edges_used;
    return sizeof(*group) + (nodes + edges) * sizeof(void *) +
        (nodes + 2 * edges) * sizeof(unsigned int) + nodes * sizeof(unsigned char);
}


/* EN: Copy group into one owned allocation, or return NULL. The caller keeps referents live.
 * RU: Скопировать group в один блок или вернуть NULL. Вызывающий код удерживает объекты. */
RecordGroup *dvm_rc_copy_record_group(const RecordGroup *group)
{
    size_t nodes = group->nodes_used;
    size_t edges = group->edges_used;
    RecordGroup *copy = malloc(dvm_rc_record_group_bytes(group));
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
    copy->live = (unsigned char *)(copy->sources + edges);
    copy->index = NULL;
    copy->owned_scratch = NULL;
    memcpy(copy->nodes, group->nodes, nodes * sizeof(*copy->nodes));
    memcpy(copy->cells, group->cells, edges * sizeof(*copy->cells));
    memcpy(copy->incoming, group->incoming, nodes * sizeof(*copy->incoming));
    memcpy(copy->targets, group->targets, edges * sizeof(*copy->targets));
    memcpy(copy->sources, group->sources, edges * sizeof(*copy->sources));
    memcpy(copy->live, group->live, nodes * sizeof(*copy->live));
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
