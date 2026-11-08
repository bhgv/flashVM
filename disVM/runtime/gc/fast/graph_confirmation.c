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


/* EN: Restore claimed counts for dead nodes below end in group.
 * RU: Восстановить счётчики захваченных мёртвых узлов group до end. */
static void restore_claimed_counts(const RecordGroup *group, unsigned int end)
{
    while (end)
    {
        unsigned int index = --end;
        if (!group->live[index])
        {
            Heap *heap = D2H(group->nodes[index]);
            if (!dvm_refcount_replace_u(&heap->ref, 0, group->incoming[index] + (index == 0)))
            {
                abort();
            }
        }
    }
}


/* EN: Claim counts for the dead nodes in group, rolling back on refusal.
 * RU: Зарезервировать счётчики мёртвых узлов group с откатом при отказе. */
static int claim_record_group(const RecordGroup *group)
{
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        if (group->live[index])
        {
            continue;
        }
        Heap *heap = D2H(group->nodes[index]);
        if (!dvm_refcount_replace_u(&heap->ref, group->incoming[index] + (index == 0), 0))
        {
            restore_claimed_counts(group, index);
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
    for (unsigned int index = 0; result == DVM_CELL_ACQUIRED && index < group->nodes_used; index++)
    {
        Heap *heap = D2H(group->nodes[index]);
        if (!group->live[index] && (dvm_runtime_root_count(heap) != 1 ||
            (heap->t == &Tchannel && !dvm_runtime_channel_is_quiet(H2D(Channel *, heap)))))
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
        restore_claimed_counts(group, group->nodes_used);
        result = DVM_CELL_REFUSED;
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return result;
}
