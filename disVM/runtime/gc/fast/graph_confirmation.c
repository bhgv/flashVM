/* Copyright (c) 2026 bhgv
 *
 * EN: Recheck captured graph edges and claim reference counts before reclamation.
 * RU: Повторная проверка рёбер и захват счётчиков ссылок перед освобождением.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "graph_confirmation.h"
#include "disvm/refcount.h"
#include "../../root_registry.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

extern int dvm_runtime_channel_is_quiet(Channel *channel); /* Native channel state check. */


/* EN: Translate a claim position into the node index of group.
 * RU: Преобразовать позицию position в индекс мёртвого узла group. */
static unsigned int dead_node_at(const RecordGroup *group, unsigned int position)
{
    return group->dead_nodes ? group->dead_nodes[position] : position;
}


/* EN: Restore counts for the first end claimed nodes in group.
 * RU: Восстановить счётчики первых end захваченных узлов group. */
static void restore_claimed_counts(const RecordGroup *group, unsigned int end)
{
    while (end)
    {
        unsigned int index = dead_node_at(group, --end);
        Heap *heap = D2H(group->nodes[index]);
        if (!dvm_refcount_replace_u(&heap->ref, 0, group->incoming[index] + (index == 0)))
        {
            abort();
        }
    }
}


/* EN: Reject a stale group before taking field guards. A successful check
 * is provisional. The locked root check and atomic claims still follow.
 * RU: Отклонить устаревший снимок group до захвата полей. Успех предварительный.
 * Далее корни проверяются под блокировками, а счётчики захватываются атомарно. */
static int counts_still_match(const RecordGroup *group)
{
    if (dvm_runtime_root_count(D2H(group->nodes[0])) != 1)
    {
        return 0;
    }
    for (unsigned int position = 0; position < group->dead_count; position++)
    {
        unsigned int index = dead_node_at(group, position);
        Heap *heap = D2H(group->nodes[index]);
        if (dvm_refcount_load_u(&heap->ref) != group->incoming[index] + (index == 0))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Claim counts for the dead nodes in group, rolling back on refusal.
 * RU: Зарезервировать счётчики мёртвых узлов group с откатом при отказе. */
static int claim_record_group(const RecordGroup *group)
{
    for (unsigned int position = 0; position < group->dead_count; position++)
    {
        unsigned int index = dead_node_at(group, position);
        Heap *heap = D2H(group->nodes[index]);
        if (!dvm_refcount_replace_u(&heap->ref, group->incoming[index] + (index == 0), 0))
        {
            restore_claimed_counts(group, position);
            return 0;
        }
    }
    return 1;
}


/* EN: Validate the retained fields of group in cells, then claim in world.
 * final_epoch receives the reader cutoff when supplied.
 * RU: Сверить сохранённые поля group в cells и захватить группу в world.
 * Если задан final_epoch, записать эпоху для ожидания читателей. */
DVM_CellResult dvm_gc_fast_confirm_graph(const RecordGroup *group, DVM_CellDomain *cells,
    DVM_NativeWorld *world, unsigned long *final_epoch)
{
    if (!counts_still_match(group))
    {
        return DVM_CELL_REFUSED;
    }
    DVM_RangeAccess access = {0};
    DVM_CellResult result = dvm_range_access_cells_begin(cells, group->cells, group->edges_used, &access);
    if (result != DVM_CELL_ACQUIRED)
    {
        return result;
    }
    for (unsigned int index = 0; index < group->edges_used; index++)
    {
        void *edge;
        memcpy(&edge, group->cells[index], sizeof(edge));
        unsigned int target = group->targets[index];
        if (edge != (target == UINT_MAX ? H : group->nodes[target]))
        {
            result = DVM_CELL_REFUSED;
            break;
        }
    }
    for (unsigned int position = 0; result == DVM_CELL_ACQUIRED && position < group->dead_count; position++)
    {
        unsigned int index = dead_node_at(group, position);
        Heap *heap = D2H(group->nodes[index]);
        if (dvm_runtime_root_count(heap) != 1 ||
            (heap->t == &Tchannel && !dvm_runtime_channel_is_quiet(H2D(Channel *, heap))))
        {
            result = DVM_CELL_REFUSED;
        }
    }
    /* New live-to-dead edges retain before publishing. A successful count
     * claim prevents any later publication into the claimed group. */
    if (result == DVM_CELL_ACQUIRED && !claim_record_group(group))
    {
        result = DVM_CELL_REFUSED;
    }
    /* Cover readers of a transient live edge that disappeared during claim. */
    if (result == DVM_CELL_ACQUIRED && final_epoch &&
        !(*final_epoch = dvm_world_retire_epoch(world)))
    {
        restore_claimed_counts(group, group->dead_count);
        result = DVM_CELL_REFUSED;
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return result;
}
