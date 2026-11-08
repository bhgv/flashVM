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


/* EN: Collect the cells of dead sources from group into selected. The caller
 * owns selected and sizes it for every edge. Return zero for invalid closure.
 * RU: Собрать поля мёртвых узлов group в буфер selected вызывающей стороны.
 * Буфер вмещает все рёбра. Вернуть ноль при незамкнутой группе. */
static int collect_dead_cells(const RecordGroup *group, void **selected, size_t *count)
{
    *count = 0;
    for (unsigned int index = 0; index < group->edges_used; index++)
    {
        unsigned int source = group->sources[index];
        unsigned int target = group->targets[index];
        if (!group->live[source])
        {
            selected[(*count)++] = group->cells[index];
        }
        else if (target != UINT_MAX && !group->live[target])
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Validate group under cells and claim it in world. Set final_epoch when requested and count skipped live fields in ignored.
 * RU: Сверить group под блокировками cells и захватить его в world.
 * Если final_epoch задан, записать эпоху. В ignored вернуть число пропущенных живых полей. */
DVM_CellResult dvm_gc_fast_confirm_graph(const RecordGroup *group, DVM_CellDomain *cells,
    DVM_NativeWorld *world, unsigned long *final_epoch, size_t *ignored)
{
    void *local[RECORD_EDGE_LIMIT];
    void **selected = group->cells;
    size_t count = group->edges_used;
    *ignored = 0;
    if (group->live_count)
    {
        selected = local;
        if (count > sizeof(local) / sizeof(*local))
        {
            selected = malloc(count * sizeof(*selected));
            if (!selected)
            {
                return DVM_CELL_REFUSED;
            }
        }
        if (!collect_dead_cells(group, selected, &count))
        {
            if (selected != local)
            {
                free(selected);
            }
            return DVM_CELL_REFUSED;
        }
    }
    DVM_RangeAccess access = {0};
    DVM_CellResult result = dvm_range_access_cells_begin(cells, selected, count, &access);
    if (result != DVM_CELL_ACQUIRED)
    {
        if (selected != local && selected != group->cells)
        {
            free(selected);
        }
        return result;
    }
    for (unsigned int index = 0; index < group->edges_used; index++)
    {
        if (group->live[group->sources[index]])
        {
            continue;
        }
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
    if (selected != local && selected != group->cells)
    {
        free(selected);
    }
    if (result == DVM_CELL_ACQUIRED)
    {
        *ignored = group->edges_used - count;
    }
    return result;
}
