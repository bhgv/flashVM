/* Copyright (c) 2026 bhgv
 * EN: Encode indexed changes and apply checked copy-back to original objects.
 * RU: Кодирование изменений по индексам и обратная запись в исходные объекты.
 */

int dvm_styx_graph_reply_write(const StyxCallShape *shape, const FVM_SRecord *baseline,
    void *const *identities, size_t count, const void *slots, size_t size,
    void *bytes, size_t capacity, size_t *used)
{
    StyxRefDefinition definitions[STYX_CALL_REF_KEYS];
    DVM_StyxRefTable refs = {.count = count, .limit = STYX_CALL_REF_KEYS, .result = 1};
    size_t originals;
    if (count > STYX_CALL_REF_KEYS ||
        !styxmanaged_ref_definitions(shape->arguments, baseline, 1, definitions, STYX_CALL_REF_KEYS, &originals) ||
        originals != count)
    {
        return 0;
    }
    if (count)
    {
        memcpy(refs.objects, identities, count * sizeof(void *));
    }
    unsigned char result[STYX_MANAGED_LIST_LIMIT], updates[STYX_MANAGED_LIST_LIMIT];
    unsigned char payload[STYX_MANAGED_LIST_LIMIT], record[STYX_MANAGED_LIST_LIMIT];
    size_t result_size, update_size, length = 0;
    int valid = !strcmp(shape->result, "()") ?
        fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, result, sizeof(result), &result_size) == FVM_S_OK :
        dvm_styx_graph_write(&refs, STYX_DYNAMIC_REPLY_TAG, shape->result, slots, size,
            result, sizeof(result), &result_size);
    for (size_t i = 0; valid && i < count; i++)
    {
        char fields[STYX_SHAPE_MAX + 1];
        unsigned char index = (unsigned char)i;
        DVM_StyxValueFrame frame = {0};
        FVM_SRecord encoded;
        size_t extent, consumed, written;
        unsigned mask;
        /* Use the same rooting and native layout checks as regular ref snapshots. */
        valid = styxmanaged_result_ref_fields(definitions[i].shape, fields) &&
            styxmanaged_layout(fields, &frame.layout) &&
            ref_layout(D2H(refs.objects[i])->t, &frame.layout) &&
            dvm_styx_value_snapshot(refs.objects[i], &frame);
        if (valid)
        {
            RefEncode encode = {.snapshot = &frame, .fields = fields, .record = record,
                .capacity = sizeof(record), .table = &refs};
            valid = kprotect(encode_snapshot, &encode) &&
                fvm_srecord_read(record, encode.size, sizeof(record), &encoded, &consumed) == FVM_S_OK &&
                consumed == encode.size &&
                fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &index, 1, encoded.payload, encoded.size,
                    updates, sizeof(updates), &extent) == FVM_S_OK &&
                fvm_srecord_read(updates, extent, sizeof(updates), &encoded, &consumed) == FVM_S_OK &&
                styxmanaged_graph_mask(&definitions[i].record, &encoded, &mask);
        }
        dvm_styx_value_frame_clear(&frame);
        if (valid && mask)
        {
            written = extent;
            valid = written <= sizeof(payload) - length;
            if (valid)
            {
                memcpy(payload + length, updates, written);
                length += written;
            }
        }
    }
    dvm_styx_ref_release(&refs, count);
    return valid &&
        fvm_srecord_write(STYX_MANAGED_GRAPH_UPDATES_TAG, payload, length, updates, sizeof(updates), &update_size) == FVM_S_OK &&
        styxcall_reply_write(STYX_MANAGED_REPLY_TAG, result, result_size, updates, update_size, bytes, capacity, used);
}


/* EN: Commit source frames directly; they own the displaced fields after swapping.
 * RU: Применить подготовленные кадры; после обмена они владеют вытесненными полями. */
int dvm_styx_graph_reply_commit(const StyxCallShape *shape, const FVM_SRecord *baseline,
    DVM_StyxRefTable *refs, void *destination, const void *bytes, size_t size)
{
    StyxRefDefinition definitions[STYX_CALL_REF_KEYS];
    StyxReplyField fields[2];
    FVM_SRecord records[2];
    size_t originals, used;
    if (!styxmanaged_ref_definitions(shape->arguments, baseline, 1, definitions, STYX_CALL_REF_KEYS, &originals) ||
        originals != refs->count || !styxcall_reply_read(STYX_MANAGED_REPLY_TAG, bytes, size, fields) ||
        !styxmanaged_graph_reply(shape, baseline, fields) ||
        fvm_srecord_read(fields[0].bytes, fields[0].size, STYX_MANAGED_LIST_LIMIT, &records[0], &used) != FVM_S_OK ||
        fvm_srecord_read(fields[1].bytes, fields[1].size, STYX_MANAGED_LIST_LIMIT, &records[1], &used) != FVM_S_OK)
    {
        return 0;
    }
    RefSwap swaps[STYX_CALL_REF_KEYS];
    DVM_StyxValueFrame *stages = records[1].size ? calloc(originals, sizeof(*stages)) : NULL;
    DVM_StyxValueFrame result = {0};
    int valid = (!records[1].size || stages) && (records[0].tag == STYX_MANAGED_NIL_TAG ||
        dvm_styx_result_fields_read(refs, shape->result, records[0].payload, records[0].size, &result));
    size_t count = 0;
    for (size_t offset = 0; valid && offset < records[1].size; offset += used)
    {
        FVM_SRecord update;
        char schema[STYX_SHAPE_MAX + 1];
        valid = fvm_srecord_read(records[1].payload + offset, records[1].size - offset,
            STYX_MANAGED_LIST_LIMIT, &update, &used) == FVM_S_OK;
        if (!valid)
        {
            break;
        }
        size_t index = update.payload[0];
        unsigned mask;
        DVM_StyxValueFrame *stage = &stages[count];
        void *original = refs->objects[index];
        valid = styxmanaged_graph_mask(&definitions[index].record, &update, &mask) && mask &&
            styxmanaged_result_ref_fields(definitions[index].shape, schema) &&
            dvm_styx_result_fields_read(refs, schema, update.payload + 1, update.size - 1, stage);
        if (valid)
        {
            swaps[count++] = (RefSwap){original, stage->words, stage->layout, mask};
            valid = ref_layout(D2H(original)->t, &stage->layout);
        }
    }
    void *moved[STYX_CALL_REF_KEYS][STYX_CALL_VALUES] = {{0}};
    if (valid)
    {
        for (size_t i = 0; i < count; i++)
        {
            unsigned roots = stages[i].roots & swaps[i].mask;
            for (size_t j = 0; j < stages[i].layout.count; j++)
            {
                if (roots & (1U << j))
                {
                    memcpy(&moved[i][j], (unsigned char *)stages[i].words + stages[i].layout.offset[j],
                        sizeof(void *));
                }
            }
        }
        valid = commit_swaps(swaps, count, destination, &result);
    }
    for (size_t i = 0; i < count; i++)
    {
        if (valid)
        {
            for (size_t j = 0; j < stages[i].layout.count; j++)
            {
                if (moved[i][j])
                {
                    ptrdel(D2H(moved[i][j]));
                    stages[i].roots &= ~(1U << j);
                }
            }
        }
        dvm_styx_value_frame_clear(&stages[i]);
    }
    free(stages);
    dvm_styx_value_frame_clear(&result);
    return valid;
}
