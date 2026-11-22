/* Copyright (c) 2026 bhgv
 * EN: Reconstruct bounded array spans against the request snapshot.
 * RU: Восстановление участков массива по снимку запроса.
 */


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_span_write(const FVM_SRecord *before, const FVM_SRecord *after,
    void *bytes, size_t capacity, size_t *used)
{
    if (before->tag != STYX_MANAGED_ARRAY_TAG || after->tag != before->tag ||
        before->size != after->size || before->size > STYX_MANAGED_LIST_LIMIT)
    {
        return 0;
    }
    size_t start = 0, end = after->size;
    while (start < end && before->payload[start] == after->payload[start])
    {
        start++;
    }
    while (end > start && before->payload[end - 1] == after->payload[end - 1])
    {
        end--;
    }
    if (start == end || end - start + 8 >= after->size)
    {
        return 0;
    }
    unsigned char header[] = {start >> 24, start >> 16, start >> 8, start};
    return fvm_srecord_write_pair(STYX_MANAGED_SPAN_TAG, header, sizeof(header),
        after->payload + start, end - start, bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_span_expand(const char *type, const FVM_SRecord *before, const FVM_SRecord *patch,
    unsigned char *scratch, size_t capacity, void *bytes, size_t limit, size_t *used)
{
    if (type[0] != 'a' || before->tag != STYX_MANAGED_ARRAY_TAG || patch->tag != STYX_MANAGED_SPAN_TAG ||
        patch->size <= 4 || before->size > capacity)
    {
        return 0;
    }
    size_t start = (uint32_t)patch->payload[0] << 24 | (uint32_t)patch->payload[1] << 16 |
        (uint32_t)patch->payload[2] << 8 | patch->payload[3];
    size_t length = patch->size - 4;
    if (start > before->size || length > before->size - start)
    {
        return 0;
    }
    memcpy(scratch, before->payload, before->size);
    memcpy(scratch + start, patch->payload + 4, length);
    return fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, scratch, before->size, bytes, limit, used) == FVM_S_OK;
}


/* EN: Rebuild the selected field stream using the corresponding original leaves.
 * RU: Восстановить выбранные поля по соответствующим исходным значениям. */
static int expand_spans(const char *schema, StyxFieldDelta *change, const FVM_SRecord *baseline,
    unsigned char *output, size_t capacity)
{
    FieldLeaves originals = {0};
    if (baseline->tag != STYX_MANAGED_REF_TAG || baseline->size < 2)
    {
        return 0;
    }
    FVM_SRecord list = *baseline;
    list.payload++;
    list.size--;
    if (!field_leaves(schema + 1, &list, &originals))
    {
        return 0;
    }
    unsigned char scratch[STYX_MANAGED_LIST_LIMIT];
    size_t input = 0, written = 0;
    for (size_t i = 0; i < originals.count; i++)
    {
        if (change->mask & (1U << i))
        {
            FVM_SRecord field;
            size_t consumed, used;
            if (input >= change->fields.size ||
                fvm_srecord_read(change->fields.payload + input, change->fields.size - input,
                    STYX_MANAGED_LIST_LIMIT, &field, &consumed) != FVM_S_OK ||
                !(field.tag == STYX_MANAGED_SPAN_TAG ?
                    styxmanaged_span_expand(originals.type[i], &originals.node[i], &field,
                        scratch, sizeof(scratch), output + written, capacity - written, &used) :
                    fvm_srecord_write(field.tag, field.payload, field.size, output + written,
                        capacity - written, &used) == FVM_S_OK))
            {
                return 0;
            }
            input += consumed;
            written += used;
        }
    }
    change->fields.payload = output;
    return input == change->fields.size && (change->fields.size = written) != 0;
}
