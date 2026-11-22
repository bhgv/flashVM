/* Copyright (c) 2026 bhgv
 * EN: Check caller reference paths under the copy-back range guards.
 * RU: Проверка путей к ссылкам клиента под блокировками обратной записи.
 */

typedef struct GraphGuard
{
    const char *shape;
    const void *slots;
    const FVM_SRecord *baseline;
    DVM_StyxRefTable *refs;
    int changed; /* A live argument path no longer matches the request. */
    DVM_BigtableEdit *tables; /* Borrowed staged table patches checked with the other fields. */
    size_t table_count; /* Number of table updates in the reply. */
} GraphGuard;

static DVM_CellResult guard_values(GraphGuard *graph, DVM_RangeAccess *access,
    const char *type, const StyxScalarLayout *layout, size_t *leaf,
    const unsigned char *slots, const FVM_SRecord *list);

static DVM_CellResult guard_ref(GraphGuard *graph, DVM_RangeAccess *access,
    const char *type, void *value, const FVM_SRecord *node)
{
    if (node->tag == STYX_MANAGED_NIL_TAG)
    {
        return value == H ? DVM_CELL_ACQUIRED : DVM_CELL_REFUSED;
    }
    unsigned index = node->payload[0];
    if (index >= graph->refs->count || value != graph->refs->objects[index])
    {
        return DVM_CELL_REFUSED;
    }
    if (node->size == 1 || *type == 'h')
    {
        return DVM_CELL_ACQUIRED;
    }
    char fields[STYX_SHAPE_MAX + 1];
    StyxScalarLayout layout;
    size_t leaf = 0;
    if (!styxmanaged_result_ref_fields(type, fields) || !styxmanaged_layout(fields, &layout))
    {
        return DVM_CELL_REFUSED;
    }
    FVM_SRecord children = {.payload = node->payload + 1, .size = node->size - 1};
    return guard_values(graph, access, fields + 1, &layout, &leaf, value, &children);
}

static DVM_CellResult guard_array(GraphGuard *graph, DVM_RangeAccess *access,
    const char *type, void *value, const FVM_SRecord *node)
{
    if (node->tag == STYX_MANAGED_NIL_TAG)
    {
        return value == H ? DVM_CELL_ACQUIRED : DVM_CELL_REFUSED;
    }
    char element[STYX_SHAPE_MAX + 1];
    StyxScalarLayout layout;
    if (!value || value == H || D2H(value)->t != &Tarray ||
        !styxmanaged_array_layout(type, element, &layout))
    {
        return DVM_CELL_REFUSED;
    }
    const Array *array = value;
    if (!array->t || array->t == H || array->t->size != layout.size || array->len < 0)
    {
        return DVM_CELL_REFUSED;
    }
    size_t offset = 0;
    for (int i = 0; i < array->len; i++)
    {
        FVM_SRecord child;
        size_t used, leaf = 0;
        if (offset >= node->size || fvm_srecord_read(node->payload + offset, node->size - offset, STYX_MANAGED_LIST_LIMIT,
                &child, &used) != FVM_S_OK)
        {
            return DVM_CELL_REFUSED;
        }
        FVM_SRecord single = {.payload = node->payload + offset, .size = used};
        DVM_CellResult claim = guard_values(graph, access, element + 1, &layout, &leaf,
            array->data + (size_t)i * layout.size, &single);
        if (claim != DVM_CELL_ACQUIRED)
        {
            return claim;
        }
        offset += used;
    }
    return offset == node->size ? DVM_CELL_ACQUIRED : DVM_CELL_REFUSED;
}

/* EN: Walk only reference-bearing paths in the validated request.
 * RU: Обойти только пути к ссылкам в проверенном запросе. */
static DVM_CellResult guard_values(GraphGuard *graph, DVM_RangeAccess *access,
    const char *type, const StyxScalarLayout *layout, size_t *leaf,
    const unsigned char *slots, const FVM_SRecord *list)
{
    size_t offset = 0;
    while (*type != ')')
    {
        const char *end = styxshape_end(type);
        FVM_SRecord node;
        size_t used;
        if (!end || offset >= list->size || fvm_srecord_read(list->payload + offset, list->size - offset,
                STYX_MANAGED_LIST_LIMIT, &node, &used) != FVM_S_OK)
        {
            return DVM_CELL_REFUSED;
        }
        DVM_CellResult claim = DVM_CELL_ACQUIRED;
        if (*type == '(')
        {
            claim = guard_values(graph, access, type + 1, layout, leaf, slots, &node);
        }
        else
        {
            size_t i = (*leaf)++;
            if (*type == 'r' || *type == 'h' || (*type == 'a' &&
                (memchr(type, 'r', (size_t)(end - type)) || memchr(type, 'h', (size_t)(end - type)))))
            {
                const void *slot = slots + layout->offset[i];
                claim = dvm_range_access_extend(access, slot, sizeof(void *));
                if (claim == DVM_CELL_ACQUIRED)
                {
                    void *value;
                    memcpy(&value, slot, sizeof(value));
                    claim = *type != 'a' ? guard_ref(graph, access, type, value, &node) :
                        guard_array(graph, access, type, value, &node);
                }
            }
        }
        if (claim != DVM_CELL_ACQUIRED)
        {
            return claim;
        }
        offset += used;
        type = end;
    }
    return DVM_CELL_ACQUIRED;
}

static DVM_CellResult guard_graph(GraphGuard *graph, DVM_RangeAccess *access)
{
    StyxScalarLayout layout;
    size_t leaf = 0;
    if (!styxmanaged_layout(graph->shape, &layout))
    {
        return DVM_CELL_REFUSED;
    }
    DVM_CellResult claim = guard_values(graph, access, graph->shape + 1, &layout, &leaf,
        graph->slots, graph->baseline);
    graph->changed = claim == DVM_CELL_REFUSED;
    return claim;
}
