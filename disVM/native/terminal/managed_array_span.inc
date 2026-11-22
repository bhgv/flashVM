/* Copyright (c) 2026 bhgv
 * EN: Reconstruct bounded array spans against the request snapshot.
 * RU: Восстановление участков массива по снимку запроса.
 */


/* EN: Encode disjoint spans when their total size beats one contiguous replacement.
 * RU: Кодировать отдельные участки, если они короче одной непрерывной замены. */
static int managed_runs_write(const FVM_SRecord *before, const FVM_SRecord *after,
    void *bytes, size_t capacity, size_t *used)
{
    if (before->tag != STYX_MANAGED_ARRAY_TAG || after->tag != before->tag ||
        before->size != after->size || before->size > STYX_MANAGED_LIST_LIMIT)
    {
        return 0;
    }
    unsigned char payload[STYX_MANAGED_LIST_LIMIT];
    size_t offset = 0, size = 0, runs = 0;
    while (offset < after->size)
    {
        if (before->payload[offset] == after->payload[offset])
        {
            offset++;
            continue;
        }
        size_t start = offset, end = ++offset;
        while (offset < after->size && offset - end < 5)
        {
            if (before->payload[offset] != after->payload[offset])
            {
                end = offset + 1;
            }
            offset++;
        }
        size_t length = end - start;
        if (length > UINT16_MAX || size + 4 + length + 4 >= after->size)
        {
            return 0;
        }
        payload[size++] = start >> 8;
        payload[size++] = start;
        payload[size++] = length >> 8;
        payload[size++] = length;
        memcpy(payload + size, after->payload + start, length);
        size += length;
        runs++;
    }
    return runs > 1 && fvm_srecord_write(STYX_MANAGED_RUN_TAG, payload, size, bytes, capacity, used) == FVM_S_OK;
}


/* EN: Apply a complete sequence of sorted, nonoverlapping nonempty runs.
 * RU: Применить полную последовательность непустых участков без пересечений. */
static int apply_runs(const FVM_SRecord *patch, unsigned char *scratch, size_t size)
{
    size_t offset = 0, previous_end = 0, runs = 0;
    while (offset < patch->size)
    {
        if (patch->size - offset < 4)
        {
            return 0;
        }
        const unsigned char *header = patch->payload + offset;
        size_t start = (size_t)header[0] << 8 | header[1];
        size_t length = (size_t)header[2] << 8 | header[3];
        offset += 4;
        if (!length || (runs && start <= previous_end) || start > size ||
            length > size - start || length > patch->size - offset)
        {
            return 0;
        }
        memcpy(scratch + start, patch->payload + offset, length);
        offset += length;
        previous_end = start + length;
        runs++;
    }
    return runs > 1;
}


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


/* EN: Represent one size change by preserving the common prefix and suffix.
 * RU: Представить изменение размера, сохранив общий префикс и суффикс. */
static int managed_splice_write(const FVM_SRecord *before, const FVM_SRecord *after,
    void *bytes, size_t capacity, size_t *used)
{
    if (before->tag != STYX_MANAGED_ARRAY_TAG || after->tag != before->tag ||
        before->size == after->size || before->size > STYX_MANAGED_LIST_LIMIT ||
        after->size > STYX_MANAGED_LIST_LIMIT)
    {
        return 0;
    }
    size_t start = 0, left = before->size, right = after->size;
    while (start < left && start < right && before->payload[start] == after->payload[start])
    {
        start++;
    }
    while (left > start && right > start && before->payload[left - 1] == after->payload[right - 1])
    {
        left--;
        right--;
    }
    size_t removed = left - start, inserted = right - start;
    if (inserted + 8 >= after->size)
    {
        return 0;
    }
    unsigned char header[] = {start >> 24, start >> 16, start >> 8, start,
        removed >> 24, removed >> 16, removed >> 8, removed};
    return fvm_srecord_write_pair(STYX_MANAGED_SPLICE_TAG, header, sizeof(header),
        after->payload + start, inserted, bytes, capacity, used) == FVM_S_OK;
}


/* EN: Read a portable unsigned offset or removal size.
 * RU: Прочитать переносимое смещение или размер удаления. */
static size_t splice_word(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 | (uint32_t)bytes[2] << 8 | bytes[3];
}


/* EN: Rebuild a splice in bounded scratch and publish only the complete record.
 * RU: Восстановить вставку или удаление в буфере и выдать только полную запись. */
static int expand_splice(const FVM_SRecord *before, const FVM_SRecord *patch,
    unsigned char *scratch, size_t capacity, void *bytes, size_t limit, size_t *used)
{
    if (patch->size < 8)
    {
        return 0;
    }
    size_t start = splice_word(patch->payload);
    size_t removed = splice_word(patch->payload + 4);
    size_t inserted = patch->size - 8;
    if (start > before->size || removed > before->size - start || removed == inserted)
    {
        return 0;
    }
    size_t retained = before->size - removed;
    if (retained > capacity || inserted > capacity - retained ||
        retained + inserted > STYX_MANAGED_LIST_LIMIT)
    {
        return 0;
    }
    memcpy(scratch, before->payload, start);
    memcpy(scratch + start, patch->payload + 8, inserted);
    memcpy(scratch + start + inserted, before->payload + start + removed, retained - start);
    return fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, scratch, retained + inserted, bytes, limit, used) == FVM_S_OK;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_span_expand(const char *type, const FVM_SRecord *before, const FVM_SRecord *patch,
    unsigned char *scratch, size_t capacity, void *bytes, size_t limit, size_t *used)
{
    if (type[0] != 'a' || before->tag != STYX_MANAGED_ARRAY_TAG || before->size > capacity ||
        patch->size <= 4 || (patch->tag != STYX_MANAGED_SPAN_TAG && patch->tag != STYX_MANAGED_RUN_TAG &&
        patch->tag != STYX_MANAGED_SPLICE_TAG))
    {
        return 0;
    }
    if (patch->tag == STYX_MANAGED_SPLICE_TAG)
    {
        return expand_splice(before, patch, scratch, capacity, bytes, limit, used);
    }
    if (patch->tag == STYX_MANAGED_RUN_TAG)
    {
        memcpy(scratch, before->payload, before->size);
        return apply_runs(patch, scratch, before->size) &&
            fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, scratch, before->size, bytes, limit, used) == FVM_S_OK;
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
                !((field.tag == STYX_MANAGED_SPAN_TAG || field.tag == STYX_MANAGED_RUN_TAG ||
                   field.tag == STYX_MANAGED_SPLICE_TAG) ?
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
