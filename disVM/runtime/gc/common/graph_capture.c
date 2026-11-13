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
#include "../../gc_fields.h"
#include "../fast/module_retirement.h"
#include "../../runtime_gc.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


extern int dvm_runtime_channel_is_quiet(Channel *channel); /* Native channel state check. */


extern int mutator;            /* Retained GC mark color, stable under shared admission. */
extern int nprop;              /* Mark propagation counter. */

#define DUKE_SLICE_FIELDS 1024 /* Fields or source nodes per admitted discovery slice. */
#define DUKE_CAPTURE_BATCH 64  /* Maximum fields before renewing discovery guards. */


/* EN: Retain and root a new target from group while its field is guarded.
 * RU: Удержать новый целевой узел group и зарегистрировать корень под блокировкой поля. */
static int pin_duke_target(RecordGroup *group, void *value)
{
    Heap *heap = D2H(value);
    if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop) || !dvm_runtime_root_add(heap))
    {
        return 0;
    }
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        if (!dvm_runtime_root_remove(heap))
        {
            abort();
        }
        group->busy = 1;
        return 0;
    }
    group->pinned_nodes++;
    return 1;
}


/* EN: End access and start a fresh discovery guard set on its domain.
 * RU: Освободить access и начать новый набор блокировок в том же домене. */
static int renew_duke_guards(RecordGroup *group, DVM_RangeAccess *access)
{
    DVM_CellDomain *domain = access->domain;
    if (!dvm_range_access_end(access))
    {
        abort();
    }
    DVM_CellResult result = dvm_range_access_begin(domain, NULL, 0, NULL, 0, access);
    group->busy = result == DVM_CELL_BUSY;
    return result == DVM_CELL_ACQUIRED;
}


/* EN: Read field cell of source index under access, or caller-owned exclusive admission. Append it to group.
 * RU: Прочитать поле cell узла index под access или эксклюзивным допуском и добавить ребро в group. */
static int record_group_edge(RecordGroup *group, unsigned int index, void *cell, DVM_RangeAccess *access)
{
    if (group->growing && group->edges_used && group->edges_used % DUKE_CAPTURE_BATCH == 0 &&
        !renew_duke_guards(group, access))
    {
        return 0;
    }
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
    unsigned int prior_nodes = group->nodes_used;
    unsigned int target = dvm_rc_record_group_index(group, edge);
    if (group->growing && group->nodes_used != prior_nodes && !pin_duke_target(group, edge))
    {
        return 0;
    }
    group->incoming[target]++;
    group->targets[group->edges_used++] = target;
    return 1;
}


/* EN: Count mapped fields in type for elements repeated values, within available slots. Store the count in mapped.
 * RU: Проверить, помещаются ли ссылочные поля elements элементов типа type в available мест. Число полей вернуть в mapped. */
static int repeated_record_fields_fit(const Type *type, size_t elements, size_t available, size_t *mapped)
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


/* EN: Capture node index in group. start_roots is the expected seed root count; access guards fields. field_end bounds Duke slices.
 * RU: Прочитать узел index в group. start_roots задаёт ожидаемое число корней начального узла, access защищает поля, field_end ограничивает порцию ДюкGC. */
static int capture_record_source(
    RecordGroup *group, unsigned int index, unsigned long start_roots, DVM_RangeAccess *access, size_t field_end)
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
            if (array->len < 0 || array->data != (unsigned char *)array + sizeof(*array))
            {
                group->live[index] = RECORD_INTRINSIC_LIVE;
                return 1;
            }
            type = array->t;
            elements = type->np ? array->len : 0;
            data = array->data;
        }
    }
    if (heap->t == &Tmodlink)
    {
        if (!dvm_gc_fast_module_eligible(heap))
        {
            group->live[index] = RECORD_INTRINSIC_LIVE;
            return 1;
        }
        group->has_modules = 1;
        Module *module = H2D(Modlink *, heap)->m;
        if (access && dvm_rc_native_module_source(group, index, module) == index && !group->source_tail_seen)
        {
            if (field_end && group->edges_used >= field_end)
            {
                return DVM_DUKE_CAPTURE_RESUME;
            }
            if (!record_group_edge(group, index, &module->origmp, access))
            {
                return 0;
            }
            group->source_tail_seen = group->growing;
        }
        type = &Tptr;
        data = (unsigned char *)&H2D(Modlink *, heap)->MP;
    }
    if (heap->t == &Tlist)
    {
        List *list = record;
        if (!group->source_tail_seen)
        {
            if (field_end && group->edges_used >= field_end)
            {
                return DVM_DUKE_CAPTURE_RESUME;
            }
            if (!record_group_edge(group, index, &list->tail, access))
            {
                return 0;
            }
            group->source_tail_seen = group->growing;
        }
        type = list->t;
        data = (unsigned char *)list->data;
        elements = type ? 1 : 0;
    }
    if (heap->t == &Tchannel)
    {
        Channel *channel = record;
        int quiet = dvm_runtime_channel_is_quiet(channel);
        if (quiet < 0)
        {
            group->busy = 1;
            return 0;
        }
        if (!quiet)
        {
            group->live[index] = RECORD_INTRINSIC_LIVE;
            return 1;
        }
        type = &Tptr;
        data = (unsigned char *)&channel->buf;
    }
    if (heap->t != &Tlist &&
        (!type || type->size < (int)sizeof(void *) ||
         (heap->t != &Tarray && heap->t != &Tchannel && heap->t != &Tmodlink && type->free != freeheap)))
    {
        group->live[index] = RECORD_INTRINSIC_LIVE;
        return 1;
    }
    unsigned long expected_roots = index == 0 ? start_roots : group->growing;
    if (!group->growing && dvm_runtime_root_count(heap) != expected_roots)
    {
        group->live[index] = 1;
    }
    if (elements > 1 && !group->source_element && !group->source_bit)
    {
        size_t mapped;
        size_t available =
            group->growing ? dvm_rc_record_group_limit() - group->edges_used : group->field_limit - group->edges_used;
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
    for (size_t element = group->source_element; element < elements; element++)
    {
        size_t first_bit = element == group->source_element ? group->source_bit : 0;
        for (int byte = first_bit / 8; byte < type->np; byte++)
        {
            unsigned int first_in_byte = byte == (int)(first_bit / 8) ? first_bit % 8 : 0;
            for (unsigned int bit = first_in_byte; bit < 8; bit++)
            {
                if (!(type->map[byte] & (0x80U >> bit)))
                {
                    continue;
                }
                size_t slot = (size_t)byte * 8 + bit;
                if (field_end && group->edges_used >= field_end)
                {
                    group->source_element = element;
                    group->source_bit = slot;
                    return DVM_DUKE_CAPTURE_RESUME;
                }
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

#include "graph_classification.inc"


/* [GC:] EN: Reacquire all captured fields, verify them, and drop discovery pins
 * while each target still has its guarded incoming reference in group.
 * Return 1 after release or 0 with pins intact on changed fields or contention.
 * No ownership escapes this shared call; access returns held only on acquisition.
 * RU: Захватить все прочитанные поля, сверить их и снять удержания узлов group
 * под защитой входящих ссылок. Результат 1 — pin сняты, 0 — отказ с сохранением
 * pin; access удержан лишь после захвата. */
static int confirm_duke_capture(RecordGroup *group, DVM_RangeAccess *access)
{
    DVM_CellDomain *domain = access->domain;
    if (!dvm_range_access_end(access))
    {
        abort();
    }
    DVM_CellResult result = dvm_range_access_cells_begin(domain, group->cells, group->edges_used, access);
    if (result != DVM_CELL_ACQUIRED)
    {
        group->busy = result == DVM_CELL_BUSY;
        return 0;
    }
    for (unsigned int index = 0; index < group->edges_used; index++)
    {
        void *value;
        memcpy(&value, group->cells[index], sizeof(value));
        unsigned int target = group->targets[index];
        if (value != (target == UINT_MAX ? H : group->nodes[target]))
        {
            group->busy = 1;
            return 0;
        }
    }
    while (group->pinned_nodes)
    {
        Heap *heap = D2H(group->nodes[group->pinned_nodes]);
        if (!dvm_refcount_drop_nonfinal_u(&heap->ref) || !dvm_runtime_root_remove(heap))
        {
            abort();
        }
        group->pinned_nodes--;
    }
    return 1;
}


/* EN: Capture from rooted start into group. start_roots gives its expected roots; access is borrowed and remains held.
 * RU: Построить group от удерживаемого start. start_roots задаёт ожидаемое число корней, access остаётся захваченным. */
int dvm_rc_queued_record_group(void *start, unsigned long start_roots, RecordGroup *group, DVM_RangeAccess *access)
{
    if (!dvm_runtime_idle_cycles || gchalt || heapmonitor)
    {
        return 0;
    }
    (void)dvm_rc_record_group_index(group, start);
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        if (!capture_record_source(group, index, start_roots, access, 0))
        {
            return 0;
        }
    }
    return classify_record_group(group, access != NULL);
}


/* EN: Capture a bounded slice from rooted start. group owns discovered pins.
 * access may hold the last batch until the caller ends it. Return 2 to resume.
 * RU: Прочитать порцию от удерживаемого start. group хранит удержанные узлы.
 * access может держать последнюю порцию. Результат 2 означает продолжение. */
int dvm_rc_duke_capture_slice(void *start, RecordGroup *group, DVM_RangeAccess *access)
{
    if (!dvm_runtime_idle_cycles || gchalt || heapmonitor)
    {
        return 0;
    }
    if (!group->nodes_used)
    {
        (void)dvm_rc_record_group_index(group, start);
    }
    size_t field_end = (size_t)group->edges_used + DUKE_SLICE_FIELDS;
    unsigned int first_node = group->next_source;
    while (group->next_source < group->nodes_used)
    {
        int result = capture_record_source(group, group->next_source, 1, access, field_end);
        if (result != 1)
        {
            if (!result && group->pinned_nodes && access->domain)
            {
                (void)confirm_duke_capture(group, access);
            }
            return result;
        }
        group->source_element = 0;
        group->source_bit = 0;
        group->source_tail_seen = 0;
        group->next_source++;
        if (group->next_source < group->nodes_used &&
            (group->edges_used >= field_end || group->next_source - first_node >= DUKE_SLICE_FIELDS))
        {
            return DVM_DUKE_CAPTURE_RESUME;
        }
    }
    if (group->roots_pruned)
    {
        /* [GC:] EN: Queue owners disappeared after root observations. Preserve
         * the seed and restart, never consume it using stale live marks.
         * RU: После чтения корней сняты владельцы очереди. Сохранить seed,
         * повторить обход; устаревшая live-метка не разрешает снять seed. */
        (void)confirm_duke_capture(group, access);
        group->busy = 1;
        return 0;
    }
    if (!classify_record_group(group, 1))
    {
        /* Unchanged guarded edges still connect every pin to the queued seed.
         * Release those pins directly; only detached pins need new candidates.
         * RU: Неизменные защищённые рёбра связывают pin с исходным seed. */
        (void)confirm_duke_capture(group, access);
        group->busy = 1;
        return 0;
    }
    if (group->live[0])
    {
        /* A live result drops the original queue owner, so verify the
         * observed graph first. Garbage candidates are rechecked by Fast. */
        return confirm_duke_capture(group, access);
    }
    if (!dvm_range_access_end(access))
    {
        abort();
    }
    return 1;
}
