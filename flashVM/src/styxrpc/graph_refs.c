/* Copyright (c) 2026 bhgv
 * EN: Apply the canonical Dis sparse graph fields to rooted native Array slots.
 * RU: Применение канонических разреженных полей графа Dis к слотам Array.
 */
#include "graph_refs.h"
#include "managed_values.h"
#include "flashvm/execution.h"
#include <stdlib.h>

typedef struct GraphLanes
{
    unsigned mask;      /* Sparse mask in flattened schema order. */
    unsigned lane;      /* Next original field lane, including unchanged lanes. */
    unsigned used;      /* Number of consumed selected values. */
    unsigned available; /* Exact selected value count. */
    FVM_Value values[FVM_STYXRPC_REF_FIELDS]; /* Rooted changed leaf values. */
} GraphLanes;


/* [GC:] EN: Encode changed fields with the same identity table as the result.
 * RU: Кодировать изменённые поля с той же таблицей идентификаторов, что и результат. */
int fvm_styxrpc_graph_updates(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const StyxRpcSchema *schema, const FVM_SRecord *arguments, unsigned originals,
    unsigned char *bytes, size_t capacity, size_t *used)
{
    unsigned char *buffer;
    unsigned i;
    size_t written = 0;
    int ok;
    *used = 0;
    if (!originals)
    {
        return 1;
    }
    buffer = malloc(2U * STYXRPC_MANAGED_LIMIT);
    ok = buffer != NULL;
    for (i = 0; ok && i < originals; ++i)
    {
        FVM_StyxRef *ref = refs->definitions + i;
        char fields[STYXRPC_MANAGED_SHAPE];
        FVM_Value current[FVM_STYXRPC_REF_FIELDS];
        FVM_SRecord after;
        size_t size, extent;
        unsigned field;
        ok = fvm_styxrpc_ref_fields(ref->type, 1, fields) &&
            fvm_styxrpc_ref_snapshot(context, ref->object, ref->count, current);
        for (field = 0; ok && field < ref->count; ++field)
        {
            ok = fvm_gc_roots_add(roots, current + field) == FVM_OK;
        }
        if (ok)
        {
            buffer[0] = (unsigned char)i;
            ok = fvm_styxrpc_values_encode(context, roots, refs, fields, current, ref->count,
                buffer + 1, STYXRPC_MANAGED_LIMIT - 1, &size);
        }
        if (ok)
        {
            after = ref->record;
            after.payload = buffer;
            after.size = size + 1;
            ok = styxrpc_graph_diff(schema, arguments, i, ref->type, &ref->record, &after,
                buffer + STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT, bytes + written,
                capacity - written, &extent);
            if (ok)
            {
                written += extent;
            }
        }
    }
    free(buffer);
    *used = written;
    return ok;
}


/* [GC:] EN: Rebuild only the tuple path to a changed lane; preserve other value identities.
 * RU: Пересобрать только путь к изменённому полю кортежа, сохраняя остальные значения. */
static int apply_lane(FVM_Context *context, FVM_GCRootScope *roots, const char *type,
    const FVM_Value *before, GraphLanes *lanes, FVM_Value *after, int *changed)
{
    const char *child;
    unsigned count, i;
    FVM_Value fields[FVM_STYXRPC_REF_FIELDS];
    FVM_Value updated[FVM_STYXRPC_REF_FIELDS];
    int any = 0;
    *after = *before;
    *changed = 0;
    if (*type != '(')
    {
        if (lanes->lane >= FVM_STYXRPC_REF_FIELDS)
        {
            return 0;
        }
        if (lanes->mask & (1U << lanes->lane))
        {
            if (lanes->used == lanes->available)
            {
                return 0;
            }
            *after = lanes->values[lanes->used++];
            *changed = 1;
        }
        ++lanes->lane;
        return 1;
    }
    count = 0;
    for (child = type + 1; *child != ')'; child = fvm_styxrpc_type_end(child))
    {
        if (++count > FVM_STYXRPC_REF_FIELDS)
        {
            return 0;
        }
    }
    if (before->type != FVM_VALUE_OBJECT || !fvm_styxrpc_ref_snapshot(context, before->object, count, fields))
    {
        return 0;
    }
    child = type + 1;
    for (i = 0; i < count; ++i, child = fvm_styxrpc_type_end(child))
    {
        int selected;
        if (!apply_lane(context, roots, child, fields + i, lanes, updated + i, &selected))
        {
            return 0;
        }
        any |= selected;
    }
    *changed = any;
    return !any || (fvm_exec_literal_array(context, count, updated, after) == FVM_OK &&
        fvm_gc_roots_add(roots, after) == FVM_OK);
}


/* [GC:] EN: Decode sparse leaves, then stage complete local direct fields.
 * RU: Декодировать разреженные поля и подготовить непосредственные слоты AS. */
static int stage_ref(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    FVM_StyxRef *ref, const FVM_SRecord *record)
{
    StyxRpcGraphFields fields;
    GraphLanes lanes = {0};
    const char *type = ref->type + 2;
    unsigned i;
    if (!styxrpc_graph_update_fields(ref->type, record, &fields) ||
        !fvm_styxrpc_values_decode(context, roots, refs, fields.shape, &fields.children, lanes.values,
            FVM_STYXRPC_REF_FIELDS, &lanes.available))
    {
        return 0;
    }
    lanes.mask = fields.mask;
    for (i = 0; i < ref->count; ++i, type = fvm_styxrpc_type_end(type))
    {
        int changed;
        if (!apply_lane(context, roots, type, ref->original + i, &lanes, ref->updated + i, &changed))
        {
            return 0;
        }
        if (changed)
        {
            ref->changed |= 1U << i;
        }
    }
    return lanes.used == lanes.available;
}


/* [GC:] EN/RU: See graph_refs.h. / См. graph_refs.h. */
int fvm_styxrpc_graph_stage(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const StyxRpcSchema *schema, const FVM_SRecord *arguments, unsigned originals,
    const FVM_SRecord *updates)
{
    size_t offset = 0;
    while (offset < updates->size)
    {
        FVM_SRecord record;
        size_t used, index;
        if (fvm_srecord_read(updates->payload + offset, updates->size - offset, STYXRPC_MANAGED_LIMIT,
                &record, &used) != FVM_S_OK ||
            !styxrpc_graph_update_index(schema, arguments, &record, originals, &index) ||
            !stage_ref(context, roots, refs, refs->definitions + index, &record))
        {
            return 0;
        }
        offset += used;
    }
    return 1;
}
