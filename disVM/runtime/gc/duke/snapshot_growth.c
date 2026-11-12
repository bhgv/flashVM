/* Copyright (c) 2026 bhgv
 *
 * EN: Heap-backed growth of Duke's graph snapshot and discovery index.
 * RU: Рост снимка графа ДюкGC в куче и перестройка индекса обхода.
 */
#include "../common/graph_snapshot.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Bound discovery fields by signed pointer span and four-slot
 * unsigned hash indexing, without a policy byte ceiling. No allocation occurs.
 * RU: Ограничить поля адресуемым диапазоном указателя и четырёхкратным
 * unsigned-индексом hash без заданного потолка памяти. Выделения нет. */
size_t dvm_rc_record_group_limit(void)
{
    const size_t per_node = 2 * sizeof(void *) + 2 * sizeof(unsigned int) + sizeof(unsigned char);
    const size_t per_field = sizeof(void *) + 10 * sizeof(unsigned int);
    size_t maximum = ((size_t)PTRDIFF_MAX - per_node) / (per_node + per_field);
    if (maximum > UINT_MAX / 4)
    {
        maximum = UINT_MAX / 4;
    }
    return maximum;
}


/* [GC:] EN: Grow private group to required fields within native representation bounds.
 * Return 1 on success, 0 with busy=1 on allocation failure or busy=0 at the
 * hard limit. Preserve indices, pins and old storage on failure. No managed
 * ownership moves; the shared collector decides retry versus hard refusal.
 * RU: Увеличить личный group до required полей в пределах native-представления. Вернуть 1
 * при успехе, 0 с busy=1 при нехватке памяти или busy=0 при жёстком пределе.
 * Отказ сохраняет индексы, pin и буфер; shared-сборщик выбирает повтор/отказ. */
int dvm_rc_grow_record_group(RecordGroup *group, size_t required)
{
    group->busy = 0;
    const size_t per_node = 2 * sizeof(void *) + 2 * sizeof(unsigned int) + sizeof(unsigned char);
    const size_t per_field = sizeof(void *) + 10 * sizeof(unsigned int);
    size_t maximum = dvm_rc_record_group_limit();
    if (!group->field_limit || group->field_limit > maximum || group->edges_used > group->field_limit ||
        group->nodes_used > group->field_limit + 1 || required > maximum)
    {
        return 0;
    }
    if (required <= group->field_limit)
    {
        return 1;
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
        group->busy = 1;
        return 0;
    }
    RecordGroup expanded = *group;
    expanded.owned_scratch = storage;
    expanded.field_limit = capacity;
    expanded.nodes = storage;
    expanded.native_modules = storage + nodes;
    expanded.cells = expanded.native_modules + nodes;
    expanded.incoming = (unsigned int *)(expanded.cells + capacity);
    expanded.native_owners = expanded.incoming + nodes;
    expanded.targets = expanded.native_owners + nodes;
    expanded.sources = expanded.targets + capacity;
    expanded.index = expanded.sources + capacity;
    expanded.native_index = expanded.index + capacity * 4;
    expanded.live = (unsigned char *)(expanded.native_index + capacity * 4);
    memcpy(expanded.nodes, group->nodes, group->nodes_used * sizeof(*expanded.nodes));
    if (group->native_modules)
    {
        memcpy(expanded.native_modules, group->native_modules, group->nodes_used * sizeof(void *));
    }
    if (group->native_owners)
    {
        memcpy(expanded.native_owners, group->native_owners, group->nodes_used * sizeof(unsigned int));
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
        if (expanded.native_modules[index])
        {
            dvm_rc_native_module_source(&expanded, index, expanded.native_modules[index]);
        }
    }
    free(group->owned_scratch);
    *group = expanded;
    return 1;
}
