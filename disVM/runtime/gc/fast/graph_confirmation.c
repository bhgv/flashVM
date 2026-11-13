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
#include "../../module_registry.h"
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

#include "module_graph_claim.inc"


#define FAST_RESERVE_BATCH 64 /* Maximum fields per preliminary guard set. */


/* EN: Validate reserved fields of group in cells. Taking each guard after
 * publication drains earlier accesses. Compare edges only when compare is set.
 * RU: Сверить зарезервированные поля group в cells. Захват каждой блокировки
 * после публикации исключает более ранние обращения. При compare сверить рёбра. */
static DVM_CellResult check_reserved_fields(const RecordGroup *group, DVM_CellDomain *cells, int compare)
{
    for (unsigned int first = 0; first < group->edges_used;)
    {
        unsigned int count = group->edges_used - first;
        if (count > FAST_RESERVE_BATCH)
        {
            count = FAST_RESERVE_BATCH;
        }
        DVM_RangeAccess access = {0};
        DVM_CellResult result = dvm_range_access_cells_begin(cells, group->cells + first, count, &access);
        if (result != DVM_CELL_ACQUIRED)
        {
            return result;
        }
        for (unsigned int offset = 0; compare && offset < count; offset++)
        {
            unsigned int index = first + offset;
            void *value;
            memcpy(&value, group->cells[index], sizeof(value));
            if (value != group->expected_fields[index])
            {
                result = DVM_CELL_REFUSED;
                break;
            }
        }
        if (!dvm_range_access_end(&access))
        {
            abort();
        }
        if (result != DVM_CELL_ACQUIRED)
        {
            return result;
        }
        first += count;
    }
    return DVM_CELL_ACQUIRED;
}


/* [GC:] EN: Validate retained group fields in cells, then claim in world.
 * final_epoch receives the reader cutoff; reserved reports successful reserved
 * claim. Propagate transient queue/cell/native allocation BUSY separately from stale-field
 * REFUSED, with all field reservations released before returning.
 * RU: Сверить поля group под cells и захватить группу в world. final_epoch
 * получает cutoff; reserved сообщает об успешном захвате под резервацией.
 * Отличать временный BUSY очереди/cells/памяти от REFUSED устаревших полей; до возврата
 * освобождать все резервации полей. */
DVM_CellResult dvm_gc_fast_confirm_graph(
    const RecordGroup *group, DVM_CellDomain *cells, DVM_NativeWorld *world, unsigned long *final_epoch, int *reserved)
{
    *reserved = 0;
    if (!counts_still_match(group))
    {
        return DVM_CELL_REFUSED;
    }
    const void *reservation = NULL;
    DVM_CellResult result = DVM_CELL_ACQUIRED;
    if (group->edges_used)
    {
        reservation = dvm_cell_reserve_fields(cells, group->cells, group->edges_used);
        if (!reservation)
        {
            return DVM_CELL_BUSY;
        }
        result = check_reserved_fields(group, cells, 1);
        if (result == DVM_CELL_ACQUIRED)
        {
            /* Drain readers that entered during the edge comparison. Writes
             * have been excluded since that comparison began. */
            dvm_cell_reservation_claim(cells, reservation);
            result = check_reserved_fields(group, cells, 0);
        }
    }
    for (unsigned int position = 0; result == DVM_CELL_ACQUIRED && position < group->dead_count; position++)
    {
        unsigned int index = dead_node_at(group, position);
        Heap *heap = D2H(group->nodes[index]);
        if (dvm_runtime_root_count(heap) != 1)
        {
            result = DVM_CELL_REFUSED;
        }
        else if (heap->t == &Tchannel)
        {
            int quiet = dvm_runtime_channel_is_quiet(H2D(Channel *, heap));
            result = quiet < 0 ? DVM_CELL_BUSY : (quiet ? DVM_CELL_ACQUIRED : DVM_CELL_REFUSED);
        }
    }
    /* New live-to-dead edges retain before publishing. A successful count
     * claim prevents any later publication into the claimed group. */
    if (result == DVM_CELL_ACQUIRED)
    {
        result = claim_graph_with_modules(group, world, final_epoch);
    }
    *reserved = result == DVM_CELL_ACQUIRED && reservation != NULL;
    if (reservation)
    {
        dvm_cell_unreserve_fields(cells, reservation);
    }
    return result;
}
