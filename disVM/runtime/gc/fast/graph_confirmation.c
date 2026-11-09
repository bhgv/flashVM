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


/* EN: Compare the captured values of group under the acquired guards.
 * RU: Сверить сохранённые значения group под захваченными блокировками. */
static int retained_fields_match(const RecordGroup *group)
{
    for (unsigned int index = 0; index < group->edges_used;)
    {
        unsigned int count = group->field_runs[index];
        if (count == 1 || memcmp(group->cells[index], &group->expected_fields[index],
                count * sizeof(void *)) != 0)
        {
            for (unsigned int offset = 0; offset < count; offset++)
            {
                void *value;
                memcpy(&value, group->cells[index + offset], sizeof(value));
                if (value != group->expected_fields[index + offset])
                {
                    return 0;
                }
            }
        }
        index += count;
    }
    return 1;
}


#define FAST_RESERVE_MIN_FIELDS 1024 /* Reserve exact fields for large snapshots. */
#define FAST_RESERVE_BATCH 64 /* Maximum fields per preliminary guard set. */


/* EN: Validate reserved fields of group in cells. Taking each guard after
 * publication drains writers that entered before the reservation was visible.
 * RU: Сверить зарезервированные поля group в cells. Захват каждой блокировки
 * после публикации исключает запись, начатую до появления резервации. */
static DVM_CellResult check_reserved_fields(const RecordGroup *group, DVM_CellDomain *cells)
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
        for (unsigned int offset = 0; offset < count; offset++)
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


/* EN: Validate the retained fields of group in cells, then claim in world.
 * final_epoch receives the reader cutoff. reserved reports a successful reservation check.
 * RU: Сверить сохранённые поля group в cells и захватить группу в world.
 * В final_epoch записать эпоху, в reserved сообщить об успешной проверке под резервацией. */
DVM_CellResult dvm_gc_fast_confirm_graph(const RecordGroup *group, DVM_CellDomain *cells,
    DVM_NativeWorld *world, unsigned long *final_epoch, int *reserved)
{
    *reserved = 0;
    if (!counts_still_match(group))
    {
        return DVM_CELL_REFUSED;
    }
    const void *reservation = NULL;
    if (group->edges_used >= FAST_RESERVE_MIN_FIELDS)
    {
        reservation = dvm_cell_reserve_fields(cells, group->cells, group->edges_used);
        if (!reservation)
        {
            return DVM_CELL_BUSY;
        }
    }
    DVM_RangeAccess access = {0};
    DVM_CellResult result;
    if (reservation)
    {
        result = check_reserved_fields(group, cells);
    }
    else
    {
        result = dvm_range_plan_begin(cells, &group->field_plan, &access);
        if (result == DVM_CELL_ACQUIRED && !retained_fields_match(group))
        {
            result = DVM_CELL_REFUSED;
        }
    }
    /* Close only captured fields to readers during count claims. Acquiring
     * each stripe after publication drains any reader that entered earlier. */
    if (reservation && result == DVM_CELL_ACQUIRED)
    {
        dvm_cell_reservation_claim(cells, reservation);
        result = check_reserved_fields(group, cells);
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
    *reserved = result == DVM_CELL_ACQUIRED && reservation != NULL;
    if (access.domain && !dvm_range_access_end(&access))
    {
        abort();
    }
    if (reservation)
    {
        dvm_cell_unreserve_fields(cells, reservation);
    }
    return result;
}
