/* Copyright (c) 2026 bhgv
 * EN: Stage and apply checked reference updates while holding caller slot guards.
 * RU: Подготовка и запись проверенных обновлений со слотами клиента под guard.
 */


typedef struct RefSwap
{
    void *original; /* Live caller ADT kept by the pending call frame. */
    void *decoded; /* Rooted private ADT carrying the new fields. */
    StyxScalarLayout layout; /* Exact native fields, excluding tail padding. */
    unsigned mask; /* Only these fields are swapped. */
} RefSwap;


/* EN: Validate every selected object and prepare disjoint targets.
 * RU: Проверить все выбранные объекты и подготовить непересекающиеся цели. */
static int prepare_swaps(const char *refs, void *const *objects, DVM_StyxValueFrame *updates, const unsigned *masks, RefSwap *swaps)
{
    const char *type = refs + 1;
    for (size_t i = 0; i < updates->layout.count; i++)
    {
        char fields[STYX_SHAPE_MAX + 1];
        StyxScalarLayout layout;
        void *decoded;
        memcpy(&decoded, (unsigned char *)updates->words + updates->layout.offset[i], sizeof(decoded));
        if (!objects[i] || objects[i] == H || !decoded || decoded == H ||
            !styxmanaged_ref_fields(type, fields) || !styxmanaged_layout(fields, &layout) ||
            !ref_layout(D2H(objects[i])->t, &layout) || !ref_layout(D2H(decoded)->t, &layout))
        {
            return 0;
        }
        for (size_t j = 0; j < i; j++)
        {
            if (objects[i] == objects[j] || decoded == swaps[j].decoded)
            {
                return 0;
            }
        }
        swaps[i] = (RefSwap){objects[i], decoded, layout, masks ? masks[i] : (1U << layout.count) - 1};
        type = styxshape_end(type);
    }
    return *type == ')';
}


#include "managed_graph_guard.inc"

/* EN: Acquire write access to the whole reply before changing any caller slot.
 * RU: Получить право записи для всего ответа до изменения первого слота. */
static int commit_swaps(RefSwap *swaps, size_t count,
    void *destination, DVM_StyxValueFrame *result, GraphGuard *graph)
{
    if (result->layout.size && (!destination || destination == H))
    {
        return 0;
    }
    DVM_RangeAccess access = {0};
    for (;;)
    {
        DVM_CellResult claim = dvm_managed_ranges_begin(NULL, 0, destination, result->layout.size, &access);
        if (claim == DVM_CELL_ACQUIRED)
        {
            for (size_t i = 0; i < count && claim == DVM_CELL_ACQUIRED; i++)
            {
                claim = dvm_range_access_extend(&access, swaps[i].original, swaps[i].layout.size);
            }
            if (graph && claim == DVM_CELL_ACQUIRED)
            {
                claim = guard_graph(graph, &access);
            }
            if (claim == DVM_CELL_ACQUIRED)
            {
                break;
            }
            if (!dvm_range_access_end(&access))
            {
                abort();
            }
        }
        if (claim != DVM_CELL_BUSY || !dvm_native_context_poll())
        {
            return 0;
        }
        fvm_port_thread_sleep_us(50);
    }
    if (graph && !dvm_bigtable_apply_edits(graph->tables, graph->table_count))
    {
        if (!dvm_range_access_end(&access))
        {
            abort();
        }
        return 0;
    }
    /* Swap ownership, leaving the displaced fields in the private decoded ADTs. */
    unsigned char old[64];
    for (size_t i = 0; i < count; i++)
    {
        RefSwap *swap = &swaps[i];
        for (size_t j = 0; j < swap->layout.count; j++)
        {
            if (!(swap->mask & (1U << j)))
            {
                continue;
            }
            size_t offset = swap->layout.offset[j], width = swap->layout.width[j];
            unsigned char *original = (unsigned char *)swap->original + offset;
            unsigned char *decoded = (unsigned char *)swap->decoded + offset;
            memcpy(old, original, width);
            memcpy(original, decoded, width);
            memcpy(decoded, old, width);
            if (swap->layout.pointers & (1U << j))
            {
                void *previous, *next;
                memcpy(&previous, old, sizeof(previous));
                memcpy(&next, original, sizeof(next));
                if (previous && previous != H)
                {
                    Setmark(D2H(previous));
                }
                if (next && next != H)
                {
                    Setmark(D2H(next));
                }
            }
        }
    }
    void *roots[STYX_CALL_VALUES] = {0};
    for (size_t i = 0; i < result->layout.count; i++)
    {
        if (result->roots & (1U << i))
        {
            memcpy(&roots[i], (unsigned char *)result->words + result->layout.offset[i], sizeof(void *));
        }
    }
    if (result->layout.size)
    {
        memcpy(old, destination, result->layout.size);
        memcpy(destination, result->words, result->layout.size);
        memcpy(result->words, old, result->layout.size);
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    for (size_t i = 0; i < result->layout.count; i++)
    {
        if (roots[i])
        {
            ptrdel(D2H(roots[i]));
        }
    }
    result->roots = 0;
    return 1;
}


static int ref_commit(const char *refs, void *const *objects, DVM_StyxValueFrame *updates,
    const unsigned *masks, void *destination, DVM_StyxValueFrame *result)
{
    RefSwap swaps[STYX_CALL_VALUES];
    return prepare_swaps(refs, objects, updates, masks, swaps) &&
        commit_swaps(swaps, updates->layout.count, destination, result, NULL);
}


/* EN/RU: See managed_ref.h. / См. managed_ref.h. */
int dvm_styx_ref_commit(const char *refs, void *const *objects, DVM_StyxValueFrame *updates,
    void *destination, DVM_StyxValueFrame *result)
{
    return ref_commit(refs, objects, updates, NULL, destination, result);
}
