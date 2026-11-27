/* Copyright (c) 2026 bhgv
 * EN: Check flattened ordinary ref updates against the original call snapshot.
 * RU: Сверить плоские обновления обычных ref со снимком исходного вызова.
 */


/* EN: Decode and validate one field mask and exactly its selected leaf values.
 * RU: Разобрать маску полей и проверить ровно выбранные ею значения. */
static int field_delta(const char *type, StyxFieldDelta *change, const FVM_SRecord *baseline,
    unsigned char *output, size_t capacity)
{
    char schema[STYX_SHAPE_MAX + 1];
    FieldLeaves leaves = {0};
    FVM_SRecord *fields = &change->fields;
    if (fields->tag != STYX_MANAGED_REF_TAG || fields->size < 2 ||
        !styxmanaged_ref_fields(type, schema) || !field_leaves(schema + 1, NULL, &leaves))
    {
        return 0;
    }
    change->mask = fields->payload[0];
    if (!change->mask || (change->mask >> leaves.count))
    {
        return 0;
    }
    select_lanes(&leaves, change->mask, change->schema);
    fields->payload++;
    fields->size--;
    if (baseline && !expand_spans(schema, change, baseline, output, capacity))
    {
        return 0;
    }
    return styxmanaged_payload_valid(change->schema, fields->payload, fields->size);
}


/* EN: Check the selector for this snapshot; normalize it for the field decoder.
 * RU: Сверить селектор этого снимка и подготовить запись для декодера полей. */
static int delta_selector(FVM_SRecord *update, const FVM_SRecord *original, const StyxGraphPositions *positions)
{
    unsigned position = original && original->tag == STYX_MANAGED_REF_TAG && original->size ?
        positions->argument[original->payload[0]] : 0;
    if (!position)
    {
        return update->tag == STYX_MANAGED_REF_TAG;
    }
    if (update->tag != STYX_MANAGED_ARG_UPDATE_TAG || update->size < 3 || update->payload[0] != position - 1)
    {
        return 0;
    }
    update->tag = STYX_MANAGED_REF_TAG;
    update->payload++;
    update->size--;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_deltas_base(const char *arguments, const void *bytes, size_t size,
    StyxFieldDelta changes[STYX_CALL_VALUES], size_t *count, const FVM_SRecord *baseline,
    unsigned char *output, size_t capacity)
{
    FVM_SRecord outer;
    char refs[STYX_SHAPE_MAX + 1];
    unsigned slots;
    size_t used;
    *count = 0;
    if (!styxmanaged_refs(arguments, refs, &slots) ||
        fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &outer, &used) != FVM_S_OK ||
        outer.tag != STYX_MANAGED_DELTA_TAG || used != size)
    {
        return 0;
    }
    if (!outer.size)
    {
        return 1;
    }
    FVM_SRecord originals[STYX_CALL_VALUES];
    size_t original_count = 0;
    StyxGraphPositions positions = {0};
    if (baseline && (!styxmanaged_ref_records(arguments, baseline, originals, &original_count) ||
        !styxmanaged_graph_positions(arguments, baseline, &positions)))
    {
        return 0;
    }
    unsigned mask = outer.payload[0], index = 0;
    size_t offset = 1;
    for (const char *type = refs + 1; *type != ')'; type = styxshape_end(type), index++)
    {
        if (mask & (1U << index))
        {
            StyxFieldDelta *change = &changes[*count];
            change->object = index;
            if (offset >= outer.size ||
                fvm_srecord_read(outer.payload + offset, outer.size - offset, STYX_MANAGED_LIST_LIMIT,
                    &change->fields, &used) != FVM_S_OK || (baseline && index >= original_count) ||
                !delta_selector(&change->fields, baseline ? &originals[index] : NULL, &positions) ||
                !field_delta(type, change, baseline ? &originals[index] : NULL, output, capacity))
            {
                return 0;
            }
            if (baseline)
            {
                output += change->fields.size;
                capacity -= change->fields.size;
            }
            (*count)++;
            offset += used;
        }
    }
    return mask && !(mask >> index) && offset == outer.size;
}




/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_deltas(const char *arguments, const void *bytes, size_t size,
    StyxFieldDelta changes[STYX_CALL_VALUES], size_t *count)
{
    return styxmanaged_deltas_base(arguments, bytes, size, changes, count, NULL, NULL, 0);
}
