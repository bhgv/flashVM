/* Copyright (c) 2026 bhgv
 *
 * EN: Heap-backed growth of Duke's graph snapshot and discovery index.
 * RU: Рост снимка графа ДюкGC в куче и перестройка индекса обхода.
 */
#include "../common/graph_snapshot.h"
#include <stdlib.h>
#include <string.h>


/* EN: Grow group to required fields within the byte budget. Preserve indices and old storage on failure.
 * RU: Увеличить group до required полей в пределах бюджета. При отказе сохранить старый буфер и индексы. */
int dvm_rc_grow_record_group(RecordGroup *group, size_t required)
{
    const size_t per_node = 2 * sizeof(void *) + sizeof(unsigned int) + sizeof(unsigned char);
    const size_t per_field = sizeof(void *) + 6 * sizeof(unsigned int);
    size_t maximum = (RECORD_SCRATCH_BYTES - per_node) / (per_node + per_field);
    if (required > maximum)
    {
        return 0;
    }
    size_t capacity = group->field_limit;
    while (capacity < required)
    {
        capacity = capacity <= maximum / 2 ? capacity * 2 : maximum;
    }
    size_t nodes = capacity + 1;
    void **storage = calloc(1, nodes * per_node + capacity * per_field);
    if (!storage)
    {
        return 0;
    }
    RecordGroup expanded = *group;
    expanded.owned_scratch = storage;
    expanded.field_limit = capacity;
    expanded.nodes = storage;
    expanded.native_modules = storage + nodes;
    expanded.cells = expanded.native_modules + nodes;
    expanded.incoming = (unsigned int *)(expanded.cells + capacity);
    expanded.targets = expanded.incoming + nodes;
    expanded.sources = expanded.targets + capacity;
    expanded.index = expanded.sources + capacity;
    expanded.live = (unsigned char *)(expanded.index + capacity * 4);
    memcpy(expanded.nodes, group->nodes, group->nodes_used * sizeof(*expanded.nodes));
    if (group->native_modules)
    {
        memcpy(expanded.native_modules, group->native_modules, group->nodes_used * sizeof(void *));
    }
    memcpy(expanded.incoming, group->incoming, group->nodes_used * sizeof(*expanded.incoming));
    memcpy(expanded.live, group->live, group->nodes_used * sizeof(*expanded.live));
    memcpy(expanded.cells, group->cells, group->edges_used * sizeof(*expanded.cells));
    memcpy(expanded.targets, group->targets, group->edges_used * sizeof(*expanded.targets));
    memcpy(expanded.sources, group->sources, group->edges_used * sizeof(*expanded.sources));
    for (unsigned int index = 0; index < expanded.nodes_used; index++)
    {
        unsigned int slot = dvm_rc_record_index_slot(&expanded, expanded.nodes[index]);
        while (expanded.index[slot])
        {
            slot = (slot + 1) % (expanded.field_limit * 4);
        }
        expanded.index[slot] = index + 1;
    }
    free(group->owned_scratch);
    *group = expanded;
    return 1;
}
