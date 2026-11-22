/* Copyright (c) 2026 bhgv
 * EN: Check bounded managed value records without a VM or a transport.
 * RU: Проверить ограниченные записи управляемых значений без VM и транспорта.
 */


/* EN: Round-trip nested strings and nil, then reject malformed UTF-8 and wrong types.
 * RU: Передать вложенные строки и nil, затем отклонить неверный UTF-8 и чужие типы. */
static int managed_records(void)
{
    const unsigned char text[] = {0xd0, 0x96, 0, 0xf0, 0x9f, 0x8c, 0x8d};
    const unsigned char bad_utf8[] = {0xed, 0xa0, 0x80};
    unsigned char children[128], payload[192], frame[256];
    size_t one, two, array, extra, size;
    FVM_SRecord decoded;
    const unsigned long tag = 89;
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, text, sizeof(text), children, sizeof(children), &one) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, children + one, sizeof(children) - one, &two) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, children, one + two, payload, sizeof(payload), &array) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_STRING_TAG, NULL, 0, payload + array, sizeof(payload) - array, &extra) != FVM_S_OK ||
        !styxmanaged_list_write(tag, "(a[s]s)", payload, array + extra, frame, sizeof(frame), &size) ||
        !styxmanaged_list_read(tag, "(a[s]s)", frame, size, &decoded) ||
        decoded.size != array + extra || memcmp(decoded.payload, payload, decoded.size) ||
        styxmanaged_list_read(tag, "(a[s]s)", frame, size - 1, &decoded) ||
        styxmanaged_list_read(tag, "(a[b]s)", frame, size, &decoded) ||
        styxmanaged_list_read(tag, "(a[s])", frame, size, &decoded) ||
        styxmanaged_list_read(tag, "(a[s]r[])", frame, size, &decoded))
    {
        return 0;
    }
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, bad_utf8, sizeof(bad_utf8), children, sizeof(children), &one) != FVM_S_OK ||
        fvm_srecord_write(tag, children, one, frame, sizeof(frame), &size) != FVM_S_OK)
    {
        return 0;
    }
    const size_t before = size;
    return !styxmanaged_list_read(tag, "(s)", frame, size, &decoded) &&
        !styxmanaged_list_write(tag, "(s)", children, one, frame, sizeof(frame), &size) &&
        size == before;
}



/* EN: Check root positions for packed bytes, nested tuples and managed references.
 * RU: Проверить позиции корней рядом с упакованными байтами и во вложенном кортеже. */
static int managed_layout(void)
{
    const char shape[] = "(ys(a[b]y)r[ib])";
    const size_t offsets[] = {0, 8, 16, 24, 32};
    StyxScalarLayout layout;
    return !styxscalar_layout(shape, &layout) && styxmanaged_layout(shape, &layout) &&
        layout.size == 40 && layout.count == 5 && layout.pointers == 0x16 &&
        !memcmp(layout.offset, offsets, sizeof(offsets));
}


/* EN: Verify independent result and update slots and reject an extra child.
 * RU: Проверить отдельные позиции результата и обновлений и отклонить третью. */
static int reply_fields(void)
{
    unsigned char result[32], updates[32], reply[96], payload[96];
    size_t a, b, used;
    StyxReplyField fields[2];
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, "abc", 3, result, sizeof(result), &a) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, NULL, 0, updates, sizeof(updates), &b) != FVM_S_OK ||
        !styxcall_reply_write(90, result, a, updates, b, reply, sizeof(reply), &used) ||
        !styxcall_reply_read(90, reply, used, fields) || fields[0].size != a || fields[1].size != b ||
        memcmp(fields[0].bytes, result, a) || memcmp(fields[1].bytes, updates, b))
    {
        return 0;
    }
    memcpy(payload, result, a);
    memcpy(payload + a, updates, b);
    memcpy(payload + a + b, updates, b);
    return fvm_srecord_write(90, payload, a + b + b, reply, sizeof(reply), &used) == FVM_S_OK &&
        !styxcall_reply_read(90, reply, used, fields) && fields[0].size == a && fields[1].size == b;
}



/* EN: Round-trip the largest scoped value and retain strict legacy limits.
 * RU: Передать предельное значение с контекстом и проверить прежний лимит. */
static int scoped_large_record(void)
{
    unsigned char payload[STYX_MANAGED_LIST_LIMIT] = {42};
    unsigned char record[STYX_MANAGED_LIST_LIMIT + STYX_SCOPED_LARGE_OVERHEAD];
    size_t used, size;
    uint32_t pid;
    const void *data;
    return styxcall_scope_write_bounded(47, payload, sizeof(payload), sizeof(payload),
            record, sizeof(record), &used) &&
        !styxcall_scope_read(record, used, &pid, &data, &size) &&
        !styxcall_scope_read_bounded(record, used, sizeof(payload) - 1, &pid, &data, &size) &&
        styxcall_scope_read_bounded(record, used, sizeof(payload), &pid, &data, &size) &&
        pid == 47 && size == sizeof(payload) && !memcmp(data, payload, size);
}


/* EN: Check sparse index selection and reject missing snapshots and out-of-range bits.
 * RU: Проверить выбор индексов и отклонить пропущенные снимки и лишние биты. */
static int managed_sparse_refs(void)
{
    const char arguments[] = "(sr[s](ir[s]))";
    unsigned char field[32], node[64], frame[128];
    size_t field_size, node_size, size;
    unsigned char index = 0, mask = 2;
    char selected[STYX_SHAPE_MAX + 1];
    unsigned decoded_mask;
    FVM_SRecord updates;
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, "new", 3, field, sizeof(field), &field_size) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &index, 1, field, field_size, node,
            sizeof(node), &node_size) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_ARRAY_TAG, &mask, 1, node, node_size, frame,
            sizeof(frame), &size) != FVM_S_OK ||
        !styxmanaged_updates(arguments, frame, size, selected, &decoded_mask, &updates) ||
        decoded_mask != mask || strcmp(selected, "(r[s])") || updates.size != node_size)
    {
        return 0;
    }
    mask = 4;
    if (fvm_srecord_write_pair(STYX_MANAGED_ARRAY_TAG, &mask, 1, node, node_size, frame,
            sizeof(frame), &size) != FVM_S_OK ||
        styxmanaged_updates(arguments, frame, size, selected, &decoded_mask, &updates))
    {
        return 0;
    }
    mask = 3;
    return fvm_srecord_write_pair(STYX_MANAGED_ARRAY_TAG, &mask, 1, node, node_size, frame,
        sizeof(frame), &size) == FVM_S_OK &&
        !styxmanaged_updates(arguments, frame, size, selected, &decoded_mask, &updates);
}


/* EN: Flatten a tuple in one changed ADT and reject a mask outside its schema.
 * RU: Развернуть кортеж изменённого ADT и отклонить маску за пределами схемы. */
static int managed_field_deltas(void)
{
    const char type[] = "r[y(sy)]";
    const char arguments[] = "(sr[y(sy)])";
    unsigned char head[32] = {0}, tuple[48], old[96], next[96], fields[96], outer[192], ref[128];
    size_t byte_size, one, two, a, b, n, r, length;
    unsigned char object = 1;
    if (!styxscalarwrite(STYX_CALL_BYTE_TAG, STYX_SCALAR_VALUE, 5, head + 1,
            sizeof(head) - 1, &byte_size) ||
        fvm_srecord_write(STYX_MANAGED_STRING_TAG, "old", 3, tuple, sizeof(tuple), &one) != FVM_S_OK ||
        !styxscalarwrite(STYX_CALL_BYTE_TAG, STYX_SCALAR_VALUE, 9, tuple + one, sizeof(tuple) - one, &two))
    {
        return 0;
    }
    memcpy(old, head, byte_size + 1);
    memcpy(next, head, byte_size + 1);
    if (fvm_srecord_write(STYX_CALL_TUPLE_TAG, tuple, one + two, old + byte_size + 1,
        sizeof(old) - byte_size - 1, &a) != FVM_S_OK)
    {
        return 0;
    }
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, "new", 3, tuple, sizeof(tuple), &one) != FVM_S_OK ||
        fvm_srecord_write(STYX_CALL_TUPLE_TAG, tuple, one + two, next + byte_size + 1,
            sizeof(next) - byte_size - 1, &b) != FVM_S_OK)
    {
        return 0;
    }
    FVM_SRecord before = {.tag = STYX_MANAGED_REF_TAG, .payload = old, .size = byte_size + 1 + a};
    FVM_SRecord after = {.tag = STYX_MANAGED_REF_TAG, .payload = next, .size = byte_size + 1 + b};
    StyxFieldDelta patches[STYX_CALL_VALUES];
    size_t count;
    if (!styxmanaged_diff_fields(type, &before, &after, fields, sizeof(fields), &n) ||
        n != 1 + one || fields[0] != 2 || memcmp(fields + 1, tuple, one) ||
        fvm_srecord_write(STYX_MANAGED_REF_TAG, fields, n, ref, sizeof(ref), &r) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_DELTA_TAG, &object, 1, ref, r,
            outer, sizeof(outer), &length) != FVM_S_OK ||
        !styxmanaged_deltas(arguments, outer, length, patches, &count) || count != 1 ||
        patches[0].object || patches[0].mask != 2 || strcmp(patches[0].schema, "(s)"))
    {
        return 0;
    }
    fields[0] = 8;
    return fvm_srecord_write(STYX_MANAGED_REF_TAG, fields, n, ref, sizeof(ref), &r) == FVM_S_OK &&
        fvm_srecord_write_pair(STYX_MANAGED_DELTA_TAG, &object, 1, ref, r,
            outer, sizeof(outer), &length) == FVM_S_OK &&
        !styxmanaged_deltas(arguments, outer, length, patches, &count);
}


/* EN: Round-trip a bounded array span and reject an offset outside the snapshot.
 * RU: Проверить восстановление участка массива и отклонить выход за границы. */
static int managed_array_span(void)
{
    unsigned char old[128], next[128], patch[128], scratch[128], array[160];
    memset(old, 3, sizeof(old));
    memcpy(next, old, sizeof(next));
    next[64] = 7;
    FVM_SRecord before = {.tag = STYX_MANAGED_ARRAY_TAG, .payload = old, .size = sizeof(old)};
    FVM_SRecord after = {.tag = STYX_MANAGED_ARRAY_TAG, .payload = next, .size = sizeof(next)};
    FVM_SRecord span, rebuilt;
    size_t packed, expanded, used;
    if (!styxmanaged_span_write(&before, &after, patch, sizeof(patch), &packed) || packed >= 16 ||
        fvm_srecord_read(patch, packed, sizeof(patch), &span, &used) != FVM_S_OK ||
        used != packed || !styxmanaged_span_expand("a[y]", &before, &span, scratch,
            sizeof(scratch), array, sizeof(array), &expanded) ||
        fvm_srecord_read(array, expanded, sizeof(array), &rebuilt, &used) != FVM_S_OK ||
        rebuilt.tag != STYX_MANAGED_ARRAY_TAG || rebuilt.size != sizeof(next) ||
        memcmp(rebuilt.payload, next, sizeof(next)))
    {
        return 0;
    }
    patch[span.payload - patch] = 255;
    return !styxmanaged_span_expand("a[y]", &before, &span, scratch, sizeof(scratch),
        array, sizeof(array), &expanded);
}


/* EN: Two distant edits should use two short runs and reject their overlap.
 * RU: Две далёкие правки должны идти двумя участками без пересечения. */
static int managed_array_runs(void)
{
    unsigned char old[512] = {0}, next[512] = {0}, a[528], b[528], fields[530], scratch[512], output[528];
    next[12] = 7;
    next[450] = 8;
    size_t an, bn, n, used;
    if (fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, old, sizeof(old), a + 1, sizeof(a) - 1, &an) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, next, sizeof(next), b + 1, sizeof(b) - 1, &bn) != FVM_S_OK)
    {
        return 0;
    }
    a[0] = b[0] = 0;
    FVM_SRecord before = {.tag = STYX_MANAGED_RUN_REQUEST_TAG, .payload = a, .size = an + 1};
    FVM_SRecord after = {.tag = STYX_MANAGED_REF_TAG, .payload = b, .size = bn + 1};
    FVM_SRecord original = {.tag = STYX_MANAGED_ARRAY_TAG, .payload = old, .size = sizeof(old)};
    FVM_SRecord patch, restored;
    if (!styxmanaged_diff_fields("r[a[y]]", &before, &after, fields, sizeof(fields), &n) ||
        n >= 20 || fields[0] != 1 ||
        fvm_srecord_read(fields + 1, n - 1, sizeof(fields), &patch, &used) != FVM_S_OK ||
        patch.tag != STYX_MANAGED_RUN_TAG || patch.size != 10 ||
        !styxmanaged_span_expand("a[y]", &original, &patch, scratch, sizeof(scratch), output, sizeof(output), &n) ||
        fvm_srecord_read(output, n, sizeof(output), &restored, &used) != FVM_S_OK ||
        restored.size != sizeof(next) || memcmp(restored.payload, next, sizeof(next)))
    {
        return 0;
    }
    unsigned char *second = fields + (patch.payload - fields) + 5;
    second[0] = 0;
    second[1] = 12;
    if (styxmanaged_span_expand("a[y]", &original, &patch, scratch, sizeof(scratch), output, sizeof(output), &n))
    {
        return 0;
    }
    before.tag = STYX_MANAGED_SPAN_REQUEST_TAG;
    return styxmanaged_diff_fields("r[a[y]]", &before, &after, fields, sizeof(fields), &n) &&
        fvm_srecord_read(fields + 1, n - 1, sizeof(fields), &patch, &used) == FVM_S_OK &&
        patch.tag == STYX_MANAGED_SPAN_TAG;
}


/* EN: Encode one interior insertion or deletion, then reject an excessive removal.
 * RU: Кодировать вставку или удаление в середине и отклонить избыточное удаление. */
static int managed_array_splice(void)
{
    unsigned char short_bytes[128], long_bytes[129], first[144], second[144], fields[148];
    unsigned char scratch[129], output[144];
    memset(short_bytes, 3, sizeof(short_bytes));
    memcpy(long_bytes, short_bytes, 64);
    long_bytes[64] = 7;
    memcpy(long_bytes + 65, short_bytes + 64, 64);
    first[0] = second[0] = 0;
    size_t a, b;
    if (fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, short_bytes, sizeof(short_bytes), first + 1,
        sizeof(first) - 1, &a) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, long_bytes, sizeof(long_bytes), second + 1,
            sizeof(second) - 1, &b) != FVM_S_OK)
    {
        return 0;
    }
    FVM_SRecord refs[] = {
        {.tag = STYX_MANAGED_SPLICE_REQUEST_TAG, .payload = first, .size = a + 1},
        {.tag = STYX_MANAGED_SPLICE_REQUEST_TAG, .payload = second, .size = b + 1},
    };
    const unsigned char *arrays[] = {short_bytes, long_bytes};
    const size_t sizes[] = {sizeof(short_bytes), sizeof(long_bytes)};
    for (size_t i = 0; i < 2; i++)
    {
        size_t n, consumed, expanded;
        FVM_SRecord patch, rebuilt;
        FVM_SRecord before = {.tag = STYX_MANAGED_ARRAY_TAG, .payload = arrays[i], .size = sizes[i]};
        if (!styxmanaged_diff_fields("r[a[y]]", &refs[i], &refs[1 - i], fields, sizeof(fields), &n) ||
            n >= 20 || fields[0] != 1 ||
            fvm_srecord_read(fields + 1, n - 1, sizeof(fields), &patch, &consumed) != FVM_S_OK ||
            patch.tag != STYX_MANAGED_SPLICE_TAG || patch.size != 9 - i ||
            !styxmanaged_span_expand("a[y]", &before, &patch, scratch, sizeof(scratch),
                output, sizeof(output), &expanded) ||
            fvm_srecord_read(output, expanded, sizeof(output), &rebuilt, &consumed) != FVM_S_OK ||
            rebuilt.size != sizes[1 - i] || memcmp(rebuilt.payload, arrays[1 - i], rebuilt.size))
        {
            return 0;
        }
        fields[patch.payload - fields + 4] = 255;
        if (styxmanaged_span_expand("a[y]", &before, &patch, scratch, sizeof(scratch),
            output, sizeof(output), &expanded))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Reassemble a UTF-8 edit before validation, including a split code point.
 * RU: Восстановить правку UTF-8 до проверки, включая участок внутри символа. */
static int managed_text_patch(void)
{
    unsigned char old[128], next[128], a[144], b[144], fields[148], scratch[128], expanded[144];
    memset(old, 'x', sizeof(old));
    memcpy(next, old, sizeof(next));
    old[50] = next[50] = 0xc3;
    old[51] = 0xa8;
    next[51] = 0xa9;
    a[0] = b[0] = 0;
    size_t an, bn, n, used, size;
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, old, sizeof(old), a + 1, sizeof(a) - 1, &an) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_STRING_TAG, next, sizeof(next), b + 1, sizeof(b) - 1, &bn) != FVM_S_OK)
    {
        return 0;
    }
    FVM_SRecord before = {.tag = STYX_MANAGED_TEXT_REQUEST_TAG, .payload = a, .size = an + 1};
    FVM_SRecord after = {.tag = STYX_MANAGED_REF_TAG, .payload = b, .size = bn + 1};
    FVM_SRecord base = {.tag = STYX_MANAGED_STRING_TAG, .payload = old, .size = sizeof(old)};
    FVM_SRecord patch;
    if (!styxmanaged_diff_fields("r[s]", &before, &after, fields, sizeof(fields), &n) ||
        n >= 16 || fields[0] != 1 ||
        fvm_srecord_read(fields + 1, n - 1, sizeof(fields), &patch, &used) != FVM_S_OK ||
        patch.tag != STYX_MANAGED_SPAN_TAG ||
        !styxmanaged_span_expand("s", &base, &patch, scratch, sizeof(scratch), expanded, sizeof(expanded), &size) ||
        !styxmanaged_payload_valid("(s)", expanded, size) || memcmp(scratch, next, sizeof(next)))
    {
        return 0;
    }
    fields[patch.payload - fields + 4] = 0xff;
    if (!styxmanaged_span_expand("s", &base, &patch, scratch, sizeof(scratch), expanded, sizeof(expanded), &size) ||
        styxmanaged_payload_valid("(s)", expanded, size))
    {
        return 0;
    }
    before.tag = STYX_MANAGED_SPLICE_REQUEST_TAG;
    return styxmanaged_diff_fields("r[s]", &before, &after, fields, sizeof(fields), &n) &&
        fvm_srecord_read(fields + 1, n - 1, sizeof(fields), &patch, &used) == FVM_S_OK &&
        patch.tag == STYX_MANAGED_STRING_TAG;
}


/* EN: Preserve a long shared interior after inserting and replacing distant bytes.
 * RU: Сохранить длинную общую середину после вставки и далёкой замены. */
static int managed_sequence_edits(void)
{
    unsigned char old[512], next[513], a[528], b[528], fields[530], scratch[513], output[528];
    for (size_t i = 0; i < sizeof(old); i++)
    {
        old[i] = i % 251;
    }
    memcpy(next, old, 12);
    next[12] = 255;
    memcpy(next + 13, old + 12, sizeof(old) - 12);
    next[451] = 254;
    a[0] = b[0] = 0;
    size_t an, bn, n, used;
    if (fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, old, sizeof(old), a + 1, sizeof(a) - 1, &an) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, next, sizeof(next), b + 1, sizeof(b) - 1, &bn) != FVM_S_OK)
    {
        return 0;
    }
    FVM_SRecord before = {.tag = STYX_MANAGED_EDIT_REQUEST_TAG, .payload = a, .size = an + 1};
    FVM_SRecord after = {.tag = STYX_MANAGED_REF_TAG, .payload = b, .size = bn + 1};
    FVM_SRecord base = {.tag = STYX_MANAGED_ARRAY_TAG, .payload = old, .size = sizeof(old)};
    FVM_SRecord patch, restored;
    if (!styxmanaged_diff_fields("r[a[y]]", &before, &after, fields, sizeof(fields), &n) ||
        n >= 24 || fields[0] != 1 ||
        fvm_srecord_read(fields + 1, n - 1, sizeof(fields), &patch, &used) != FVM_S_OK ||
        patch.tag != STYX_MANAGED_EDIT_TAG || patch.size != 14 ||
        !styxmanaged_span_expand("a[y]", &base, &patch, scratch, sizeof(scratch), output, sizeof(output), &n) ||
        fvm_srecord_read(output, n, sizeof(output), &restored, &used) != FVM_S_OK ||
        restored.size != sizeof(next) || memcmp(restored.payload, next, sizeof(next)))
    {
        return 0;
    }
    fields[patch.payload - fields + 7] = 0;
    fields[patch.payload - fields + 8] = 5;
    if (styxmanaged_span_expand("a[y]", &base, &patch, scratch, sizeof(scratch), output, sizeof(output), &n))
    {
        return 0;
    }
    before.tag = STYX_MANAGED_TEXT_REQUEST_TAG;
    return styxmanaged_diff_fields("r[a[y]]", &before, &after, fields, sizeof(fields), &n) &&
        fvm_srecord_read(fields + 1, n - 1, sizeof(fields), &patch, &used) == FVM_S_OK &&
        patch.tag == STYX_MANAGED_SPLICE_TAG;
}


/* EN: Verify capability negotiation and exclude profile and reply tags.
 * RU: Проверить согласование возможностей и исключить теги профиля и ответа. */
static int managed_capabilities(void)
{
    const unsigned long tags[] = {STYX_MANAGED_DELTA_REQUEST_TAG, STYX_MANAGED_SPAN_REQUEST_TAG,
        STYX_MANAGED_RUN_REQUEST_TAG, STYX_MANAGED_SPLICE_REQUEST_TAG,
        STYX_MANAGED_TEXT_REQUEST_TAG, STYX_MANAGED_EDIT_REQUEST_TAG, STYX_MANAGED_WIDE_REFS_TAG, STYX_MANAGED_GRAPH_TAG};
    for (size_t i = 0; i < sizeof(tags) / sizeof(tags[0]); i++)
    {
        if (styxmanaged_capability(0, i) != tags[i] || !styxmanaged_request_tag(tags[i]) ||
            styxmanaged_has_snapshot(tags[i]) != (i != 0 && tags[i] != STYX_MANAGED_GRAPH_TAG))
        {
            return 0;
        }
    }
    return styxmanaged_request_tag(STYX_MANAGED_REQUEST_TAG) &&
        !styxmanaged_has_snapshot(STYX_MANAGED_REQUEST_TAG) &&
        !styxmanaged_request_tag(STYX_MANAGED_REPLY_TAG) &&
        !styxmanaged_request_tag(STYX_SCOPED_RECORD_OFFER_TAG) &&
        !styxmanaged_request_tag(STYX_MANAGED_GRAPH_TAG + 1) &&
        styxmanaged_capability(0, STYX_MANAGED_GRAPH_VERSION + 1) == STYX_MANAGED_DELTA_REQUEST_TAG &&
        styxmanaged_capability(1, 0) == STYX_MANAGED_DELTA_REQUEST_TAG;
}


/* EN: Match a returned index against the request definition and its exact schema.
 * RU: Сверить индекс результата с определением из запроса и его точной схемой. */
static int linked_result_refs(void)
{
    unsigned char text[32], request[64], alias[32], wire[48];
    unsigned char index = 0;
    size_t length, request_size, alias_size, wire_size;
    FVM_SRecord result;
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, "seed", 4, text, sizeof(text), &length) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &index, 1, text, length,
            request, sizeof(request), &request_size) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_REF_TAG, &index, 1, alias, sizeof(alias), &alias_size) != FVM_S_OK ||
        fvm_srecord_write(STYX_DYNAMIC_REPLY_TAG, alias, alias_size, wire, sizeof(wire), &wire_size) != FVM_S_OK)
    {
        return 0;
    }
    FVM_SRecord arguments = {.tag = STYX_MANAGED_REQUEST_TAG, .payload = request, .size = request_size};
    if (!styxmanaged_result_read("(r[s])", wire, wire_size, "(r[s])", &arguments, &result) ||
        styxmanaged_result_read("(r[i])", wire, wire_size, "(r[s])", &arguments, &result) ||
        styxmanaged_list_read(STYX_DYNAMIC_REPLY_TAG, "(r[s])", wire, wire_size, &result))
    {
        return 0;
    }
    index = 1;
    return fvm_srecord_write(STYX_MANAGED_REF_TAG, &index, 1, alias, sizeof(alias), &alias_size) == FVM_S_OK &&
        fvm_srecord_write(STYX_DYNAMIC_REPLY_TAG, alias, alias_size, wire, sizeof(wire), &wire_size) == FVM_S_OK &&
        !styxmanaged_result_read("(r[s])", wire, wire_size, "(r[s])", &arguments, &result);
}


/* EN: Validate a nested definition and reject a forward alias in its place.
 * RU: Проверить вложенное определение и отклонить ссылку до определения. */
static int linked_result_graph(void)
{
    unsigned char field[24], definition[48], alias[24], fields[96], object[112], result[128];
    unsigned char child = 1, parent = 0;
    size_t field_size, definition_size, alias_size, object_size, result_size;
    FVM_SRecord empty = {0}, decoded;
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, "child", 5, field, sizeof(field), &field_size) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &child, 1, field, field_size,
            definition, sizeof(definition), &definition_size) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_REF_TAG, &child, 1, alias, sizeof(alias), &alias_size) != FVM_S_OK)
    {
        return 0;
    }
    for (int reverse = 0; reverse < 2; reverse++)
    {
        size_t first = reverse ? alias_size : definition_size;
        memcpy(fields, reverse ? alias : definition, first);
        memcpy(fields + first, reverse ? definition : alias, reverse ? definition_size : alias_size);
        if (fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &parent, 1, fields, definition_size + alias_size,
                object, sizeof(object), &object_size) != FVM_S_OK ||
            fvm_srecord_write(STYX_DYNAMIC_REPLY_TAG, object, object_size, result,
                sizeof(result), &result_size) != FVM_S_OK ||
            styxmanaged_result_read("(r[r[s]r[s]])", result, result_size, "()", &empty, &decoded) != !reverse)
        {
            return 0;
        }
    }
    static const char schema[] = "(r[r[s]r[s]])";
    StyxRefDefinition definitions[2];
    size_t count;
    /* The last iteration encoded a forward alias, with a complete definition later. */
    FVM_SRecord graph = {.payload = object, .size = object_size};
    if (styxmanaged_ref_definitions(schema, &graph, 1, definitions, 2, &count))
    {
        return 0;
    }
    memcpy(fields, definition, definition_size);
    memcpy(fields + definition_size, alias, alias_size);
    if (fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &parent, 1, fields, definition_size + alias_size,
            object, sizeof(object), &graph.size) != FVM_S_OK ||
        styxmanaged_ref_definitions(schema, &graph, 0, definitions, 2, &count) ||
        styxmanaged_ref_definitions(schema, &graph, 1, definitions, 1, &count) ||
        !styxmanaged_ref_definitions(schema, &graph, 1, definitions, 2, &count) || count != 2 ||
        definitions[0].shape != schema + 1 || definitions[1].shape != schema + 3 ||
        definitions[0].record.payload[0] != parent || definitions[1].record.payload[0] != child)
    {
        return 0;
    }
    return 1;
}


/* EN: Check nested arguments, a typed result alias, and duplicate update rejection.
 * RU: Проверить вложенные аргументы, тип ссылки результата и отказ при дубликате обновления. */
static int graph_reply_records(void)
{
    StyxCallShape shape = {.arguments = "(a[r[s]])", .result = "(r[s])"};
    unsigned char text[24], object[32], request[64], alias[16], reply[32], payload[64], updates[80];
    unsigned char index = 0;
    size_t text_size, object_size, alias_size, used;
    FVM_SRecord arguments;
    StyxReplyField fields[2] = {{reply, 0}, {updates, 0}};
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, "original", 8, text, sizeof(text), &text_size) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &index, 1, text, text_size, object,
            sizeof(object), &object_size) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, object, object_size, payload, sizeof(payload), &used) != FVM_S_OK ||
        !styxmanaged_list_write(STYX_MANAGED_GRAPH_TAG, shape.arguments, payload, used, request, sizeof(request), &used) ||
        !styxmanaged_list_read(STYX_MANAGED_GRAPH_TAG, shape.arguments, request, used, &arguments) ||
        !styxmanaged_graph_required(shape.arguments) || styxmanaged_graph_required("(r[s])") ||
        fvm_srecord_write(STYX_MANAGED_REF_TAG, &index, 1, alias, sizeof(alias), &alias_size) != FVM_S_OK ||
        fvm_srecord_write(STYX_DYNAMIC_REPLY_TAG, alias, alias_size, reply, sizeof(reply), &fields[0].size) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_GRAPH_UPDATES_TAG, object, object_size, updates,
            sizeof(updates), &fields[1].size) != FVM_S_OK ||
        !styxmanaged_graph_reply(&shape, &arguments, fields))
    {
        return 0;
    }
    shape.result[3] = 'i';
    if (styxmanaged_graph_reply(&shape, &arguments, fields))
    {
        return 0;
    }
    shape.result[3] = 's';
    memcpy(payload, object, object_size);
    memcpy(payload + object_size, object, object_size);
    return fvm_srecord_write(STYX_MANAGED_GRAPH_UPDATES_TAG, payload, object_size * 2,
        updates, sizeof(updates), &fields[1].size) == FVM_S_OK &&
        !styxmanaged_graph_reply(&shape, &arguments, fields);
}
