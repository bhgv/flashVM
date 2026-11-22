/* Copyright (c) 2026 bhgv
 * EN: Build the reply's updates for original argument objects.
 * RU: Формирование раздела обновлений исходных объектов аргументов.
 */


/* EN: Encode changed original ADTs once, excluding aliases and nil arguments.
 * RU: Кодировать изменившиеся исходные ADT один раз, исключая дубликаты и nil. */
int dvm_styx_ref_updates(const char *arguments, const void *slots, const FVM_SRecord *baseline,
    void *bytes, size_t capacity, size_t *used)
{
    char refs[STYX_SHAPE_MAX + 1];
    void *objects[STYX_CALL_VALUES];
    if (!dvm_styx_ref_slots(arguments, slots, refs, objects))
    {
        return 0;
    }
    if (!strcmp(refs, "()"))
    {
        unsigned long tag = baseline->tag != STYX_MANAGED_REQUEST_TAG ?
            STYX_MANAGED_DELTA_TAG : STYX_MANAGED_ARRAY_TAG;
        return fvm_srecord_write(tag, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    FVM_SRecord before[STYX_CALL_VALUES], after[STYX_CALL_VALUES], frame;
    size_t count, other_count, extent, consumed;
    unsigned char *encoded = malloc(capacity);
    unsigned char *payload = malloc(capacity);
    StyxScalarLayout layout;
    int valid = encoded && payload && styxmanaged_layout(refs, &layout) &&
        styxmanaged_ref_records(arguments, baseline, before, &count) &&
        dvm_styx_value_frame_write(STYX_MANAGED_REQUEST_TAG, refs, objects, layout.size,
            encoded, capacity, &extent) &&
        fvm_srecord_read(encoded, extent, capacity, &frame, &consumed) == FVM_S_OK &&
        consumed == extent && styxmanaged_ref_records(refs, &frame, after, &other_count) && count == other_count;
    unsigned char index = 0, mask = 0;
    size_t length = 1;
    int delta = baseline->tag != STYX_MANAGED_REQUEST_TAG;
    unsigned char *fields = delta ? malloc(capacity) : NULL;
    valid = valid && (!delta || fields);
    const char *type = refs + 1;
    for (size_t i = 0; valid && i < count; i++, type = styxshape_end(type))
    {
        if (after[i].tag != STYX_MANAGED_REF_TAG || after[i].size <= 1 ||
            (before[i].size == after[i].size && !memcmp(before[i].payload, after[i].payload, after[i].size)))
        {
            continue;
        }
        size_t written;
        if (delta)
        {
            size_t field_size;
            before[i].tag = baseline->tag;
            valid = styxmanaged_diff_fields(type, &before[i], &after[i], fields, capacity, &field_size);
            if (!valid || !field_size)
            {
                continue;
            }
            valid = length <= capacity && fvm_srecord_write(STYX_MANAGED_REF_TAG, fields, field_size,
                payload + length, capacity - length, &written) == FVM_S_OK;
        }
        else
        {
            valid = length <= capacity &&
                fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &index, 1,
                    after[i].payload + 1, after[i].size - 1, payload + length, capacity - length,
                    &written) == FVM_S_OK;
        }
        if (valid)
        {
            mask |= 1U << i;
            length += written;
            index++;
        }
    }
    if (valid && mask)
    {
        payload[0] = mask;
    }
    valid = valid && fvm_srecord_write(delta ? STYX_MANAGED_DELTA_TAG : STYX_MANAGED_ARRAY_TAG,
        payload, mask ? length : 0,
        bytes, capacity, used) == FVM_S_OK;
    free(fields);
    free(payload);
    free(encoded);
    return valid;
}
