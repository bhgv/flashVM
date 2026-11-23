/* Copyright (c) 2026 bhgv
 * EN: Validate managed value trees and call-local reference identities.
 * RU: Проверка деревьев значений и идентичности ссылок внутри вызова.
 */


#include "managed_ref_validation.inc"


/* EN: Traverse one nested value, checking tag, payload and bounds before advancing.
 * RU: Проверить тег, содержимое и границы вложенного значения до продвижения. */
static int managed_value(const char *shape, const unsigned char *bytes, size_t size, unsigned depth, size_t *used, RefKeys *keys)
{
    FVM_SRecord value;
    if (depth == 6 || fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &value, used) != FVM_S_OK)
    {
        return 0;
    }
    const char type = *shape;
    if ((type == 's' || type == 'a' || type == 'r' || type == 'h' || type == 'c') &&
        value.tag == STYX_MANAGED_NIL_TAG)
    {
        return value.size == 0;
    }
    switch (type)
    {
    case 'c':
        return value.tag == STYX_MANAGED_CHANNEL_TAG && value.size == 4 && value.payload[0] < 128 &&
            (value.payload[0] || value.payload[1] || value.payload[2] || value.payload[3]);
    case 'r':
    case 'h':
        return managed_ref(shape, &value, depth, keys);
    case 's':
        return value.tag == STYX_MANAGED_STRING_TAG &&
            fvm_utf8_validate((const char *)value.payload, value.size, NULL) == FVM_UTF8_OK;
    case 'a':
        return value.tag == STYX_MANAGED_ARRAY_TAG &&
            managed_children(shape + 2, value.payload, value.size, 1, depth + 1, keys);
    case '(':
        return value.tag == STYX_CALL_TUPLE_TAG && managed_children(shape + 1, value.payload, value.size, 0, depth + 1, keys);
    case 'i':
    case 'b':
    case 'f':
    case 'y':
    {
        StyxScalarKind kind;
        int64_t scalar;
        return styxscalarread(node_tag(type), bytes, *used, &kind, &scalar) &&
            kind == STYX_SCALAR_VALUE && scalar_valid(type, scalar);
    }
    default:
        return 0;
    }
}


/* EN: Scan borrowed child records once. Shape is already checked as a complete list.
 * RU: Один раз пройти заимствованные записи. Схема уже проверена целиком. */
static int managed_children(const char *shape, const unsigned char *bytes, size_t size, int array,
    unsigned depth, RefKeys *keys)
{
    while (array ? size != 0 : *shape != ')' && *shape != ']')
    {
        size_t used;
        if (!managed_value(shape, bytes, size, depth, &used, keys))
        {
            return 0;
        }
        if (!array)
        {
            shape = styxshape_end(shape);
        }
        bytes += used;
        size -= used;
    }
    return size == 0;
}


/* EN: Apply the schema grammar with a caller-owned table of borrowed type names.
 * RU: Проверить грамматику с таблицей заимствованных имён типов вызывающего. */
static int managed_payload(const char *shape, const void *bytes, size_t size, RefKeys *keys)
{
    StyxCallShape schema = {.result = "()"};
    if (!shape || (size && !bytes))
    {
        return 0;
    }
    size_t length = strlen(shape);
    if (length > STYX_SHAPE_MAX)
    {
        return 0;
    }
    memcpy(schema.arguments, shape, length + 1);
    return styxshapevalid(&schema) && managed_children(shape + 1, bytes, size, 0, 0, keys);
}


#include "managed_result_validation.inc"


/* EN: Find ref records in leaf order, descending only through tuple nodes.
 * RU: Найти записи ref в порядке полей, спускаясь только в кортежи. */
static int collect_refs(const char *type, const unsigned char *bytes, size_t size, FVM_SRecord *refs, size_t *count)
{
    while (*type != ')')
    {
        FVM_SRecord node;
        size_t used;
        if (fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &node, &used) != FVM_S_OK)
        {
            return 0;
        }
        if (*type == '(' && !collect_refs(type + 1, node.payload, node.size, refs, count))
        {
            return 0;
        }
        if (*type == 'r')
        {
            if (*count >= STYX_CALL_VALUES)
            {
                return 0;
            }
            refs[(*count)++] = node;
        }
        type = styxshape_end(type);
        bytes += used;
        size -= used;
    }
    return size == 0;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_ref_records(const char *shape, const FVM_SRecord *list, FVM_SRecord *refs, size_t *count)
{
    *count = 0;
    return styxmanaged_payload_valid(shape, list->payload, list->size) &&
        collect_refs(shape + 1, list->payload, list->size, refs, count);
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_updates(const char *arguments, const void *bytes, size_t size,
    char selected[STYX_SHAPE_MAX + 1], unsigned *mask, FVM_SRecord *updates)
{
    char refs[STYX_SHAPE_MAX + 1];
    unsigned slots;
    size_t used;
    if (!styxmanaged_refs(arguments, refs, &slots) ||
        fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, updates, &used) != FVM_S_OK ||
        updates->tag != STYX_MANAGED_ARRAY_TAG || used != size)
    {
        return 0;
    }
    *mask = updates->size ? updates->payload[0] : 0;
    if (updates->size)
    {
        if (!*mask)
        {
            return 0;
        }
        updates->payload++;
        updates->size--;
    }
    size_t length = 1;
    unsigned index = 0;
    selected[0] = '(';
    for (const char *type = refs + 1; *type != ')'; index++)
    {
        const char *end = styxshape_end(type);
        if (*mask & (1U << index))
        {
            size_t n = (size_t)(end - type);
            memcpy(selected + length, type, n);
            length += n;
        }
        type = end;
    }
    selected[length++] = ')';
    selected[length] = 0;
    if ((*mask >> index) || !styxmanaged_payload_valid(selected, updates->payload, updates->size))
    {
        return 0;
    }
    const unsigned char *cursor = updates->payload;
    size_t remain = updates->size;
    for (unsigned id = 0; remain; id++)
    {
        FVM_SRecord node;
        if (fvm_srecord_read(cursor, remain, STYX_MANAGED_LIST_LIMIT, &node, &used) != FVM_S_OK ||
            node.tag != STYX_MANAGED_REF_TAG || node.size < 2 || node.payload[0] != id)
        {
            return 0;
        }
        cursor += used;
        remain -= used;
    }
    return 1;
}


typedef struct FieldLeaves
{
    const char *type[STYX_CALL_VALUES]; /* Borrowed beginning of each flattened type. */
    size_t length[STYX_CALL_VALUES]; /* Length of each type token. */
    FVM_SRecord node[STYX_CALL_VALUES]; /* Borrowed leaf records when a snapshot is supplied. */
    size_t count; /* Number of visited leaves. */
} FieldLeaves;


/* EN: Flatten a validated field schema and optionally its matching snapshot.
 * RU: Развернуть проверенную схему полей и, если задан, её снимок. */
static int field_leaves(const char *type, const FVM_SRecord *list, FieldLeaves *leaves)
{
    size_t offset = 0;
    while (*type != ')')
    {
        FVM_SRecord node = {0};
        size_t used = 0;
        const char *end = styxshape_end(type);
        if (!end || (list && (offset >= list->size ||
            fvm_srecord_read(list->payload + offset, list->size - offset,
                STYX_MANAGED_LIST_LIMIT, &node, &used) != FVM_S_OK)))
        {
            return 0;
        }
        if (*type == '(')
        {
            if (!field_leaves(type + 1, list ? &node : NULL, leaves))
            {
                return 0;
            }
        }
        else
        {
            if (leaves->count >= STYX_CALL_VALUES)
            {
                return 0;
            }
            size_t i = leaves->count++;
            leaves->type[i] = type;
            leaves->length[i] = (size_t)(end - type);
            leaves->node[i] = node;
        }
        offset += used;
        type = end;
    }
    return !list || offset == list->size;
}


/* EN: Build the compact tuple schema in native slot order.
 * RU: Собрать компактную схему кортежа в порядке нативных слотов. */
static void select_lanes(const FieldLeaves *leaves, unsigned mask, char *selected)
{
    size_t length = 1;
    selected[0] = '(';
    for (size_t i = 0; i < leaves->count; i++)
    {
        if (mask & (1U << i))
        {
            memcpy(selected + length, leaves->type[i], leaves->length[i]);
            length += leaves->length[i];
        }
    }
    selected[length++] = ')';
    selected[length] = 0;
}


#include "managed_graph_validation.inc"
#include "managed_array_span.inc"


/* EN: Choose the smallest supported patch before falling back to the complete leaf.
 * RU: Выбрать компактный поддерживаемый патч либо оставить полное значение. */
static int field_patch(char type, unsigned long version, const FVM_SRecord *old, const FVM_SRecord *next,
    void *output, size_t capacity, size_t *used)
{
    int edits = version == STYX_MANAGED_EDIT_REQUEST_TAG || version == STYX_MANAGED_WIDE_REFS_TAG ||
        version == STYX_MANAGED_POSITION_FLAT_TAG;
    int text = edits || version == STYX_MANAGED_TEXT_REQUEST_TAG;
    int splice = text || version == STYX_MANAGED_SPLICE_REQUEST_TAG;
    int runs = splice || version == STYX_MANAGED_RUN_REQUEST_TAG;
    int span = runs || version == STYX_MANAGED_SPAN_REQUEST_TAG;
    if (type != 'a' && !(type == 's' && text))
    {
        return 0;
    }
    size_t standard;
    int known = (splice && managed_splice_write(old, next, output, capacity, &standard)) ||
        (runs && managed_runs_write(old, next, output, capacity, &standard)) ||
        (span && styxmanaged_span_write(old, next, output, capacity, &standard));
    if (edits && managed_edits_write(old, next, output, known ? standard - 1 : capacity, used))
    {
        return 1;
    }
    if (known)
    {
        *used = standard;
    }
    return known;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_diff_fields(const char *type, const FVM_SRecord *before, const FVM_SRecord *after,
    void *bytes, size_t capacity, size_t *used)
{
    char schema[STYX_SHAPE_MAX + 1];
    FieldLeaves left = {0}, right = {0};
    if (before->size < 2 || after->size < 2 || !styxmanaged_ref_fields(type, schema))
    {
        return 0;
    }
    FVM_SRecord a = *before, b = *after;
    a.payload++;
    a.size--;
    b.payload++;
    b.size--;
    if (!field_leaves(schema + 1, &a, &left) || !field_leaves(schema + 1, &b, &right))
    {
        return 0;
    }
    unsigned char *output = bytes;
    size_t size = 1;
    unsigned char mask = 0;
    for (size_t i = 0; i < right.count; i++)
    {
        const FVM_SRecord *old = &left.node[i], *next = &right.node[i];
        if (old->tag == next->tag && old->size == next->size && !memcmp(old->payload, next->payload, next->size))
        {
            continue;
        }
        size_t n;
        if (size >= capacity ||
            (!field_patch(*right.type[i], before->tag, old, next, output + size, capacity - size, &n) &&
            fvm_srecord_write(next->tag, next->payload, next->size,
                output + size, capacity - size, &n) != FVM_S_OK))
        {
            return 0;
        }
        size += n;
        mask |= 1U << i;
    }
    if (mask)
    {
        output[0] = mask;
    }
    *used = mask ? size : 0;
    return 1;
}


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
