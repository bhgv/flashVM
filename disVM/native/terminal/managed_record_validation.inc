/* Copyright (c) 2026 bhgv
 * EN: Validate managed value trees and call-local reference identities.
 * RU: Проверка деревьев значений и идентичности ссылок внутри вызова.
 */


typedef struct RefKeys
{
    const char *type[STYX_CALL_VALUES]; /* First occurrence of each object schema. */
    size_t count; /* Number of snapshots already defined. */
} RefKeys;


/* EN: Validate one tuple's exact arity or one array's homogeneous record sequence.
 * RU: Проверить точное число полей кортежа или однородную последовательность массива. */
static int managed_children(const char *shape, const unsigned char *bytes, size_t size, int array,
    unsigned depth, RefKeys *keys);


/* EN: Extract the fields of a ref ADT as a value-list schema.
 * RU: Получить схему списка полей объекта ref ADT. */
int styxmanaged_ref_fields(const char *shape, char fields[STYX_SHAPE_MAX + 1])
{
    const char *end = shape && *shape == 'r' ? styxshape_end(shape) : NULL;
    if (!end || end - shape <= 3 || end - shape > STYX_SHAPE_MAX)
    {
        return 0;
    }
    size_t n = (size_t)(end - shape) - 3;
    if (memchr(shape + 2, 'r', n))
    {
        return 0;
    }
    fields[0] = '(';
    memcpy(fields + 1, shape + 2, n);
    fields[n + 1] = ')';
    fields[n + 2] = 0;
    return 1;
}


/* EN: Claim a new reference index or verify an earlier index with the same schema.
 * RU: Принять новый индекс ссылки либо сверить схему ранее объявленного индекса. */
static int managed_ref(const char *shape, const FVM_SRecord *value, unsigned depth, RefKeys *keys)
{
    if (value->tag != STYX_MANAGED_REF_TAG || !value->size)
    {
        return 0;
    }
    size_t index = value->payload[0];
    if (index < keys->count)
    {
        const char *first = keys->type[index];
        size_t length = (size_t)(styxshape_end(first) - first);
        return value->size == 1 && (size_t)(styxshape_end(shape) - shape) == length &&
            !memcmp(shape, first, length);
    }
    char fields[STYX_SHAPE_MAX + 1];
    if (index != keys->count || index >= STYX_CALL_VALUES ||
        !styxmanaged_ref_fields(shape, fields) ||
        !managed_children(fields + 1, value->payload + 1, value->size - 1, 0, depth + 1, keys))
    {
        return 0;
    }
    keys->type[keys->count++] = shape;
    return 1;
}


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
    if ((type == 's' || type == 'a' || type == 'r') && value.tag == STYX_MANAGED_NIL_TAG)
    {
        return value.size == 0;
    }
    switch (type)
    {
    case 'r':
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
    while (array ? size != 0 : *shape != ')')
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


/* EN: Apply the compiled schema grammar including call-local reference identity.
 * RU: Проверить грамматику схемы Dis и идентичность ref внутри записи. */
int styxmanaged_payload_valid(const char *shape, const void *bytes, size_t size)
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
    RefKeys keys = {0};
    return styxshapevalid(&schema) && managed_children(schema.arguments + 1, bytes, size, 0, 0, &keys);
}




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
