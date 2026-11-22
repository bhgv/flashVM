/* Copyright (c) 2026 bhgv
 * EN: Validate bounded reference definitions and aliases.
 * RU: Проверка ограниченных определений и алиасов ссылок.
 */


typedef struct RefKeys
{
    const char *type[STYX_CALL_REF_KEYS]; /* First occurrence of each object schema. */
    StyxRefDefinition *definitions; /* Optional definitions indexed by wire identity. */
    size_t count; /* Number of reserved snapshot indices. */
    size_t limit; /* Maximum count, or zero for the standalone limit. */
    int nested; /* Result traversal may define references inside values. */
} RefKeys;


/* EN: Validate one tuple's exact arity or one array's homogeneous record sequence.
 * RU: Проверить точное число полей кортежа или однородную последовательность массива. */
static int managed_children(const char *shape, const unsigned char *bytes, size_t size, int array,
    unsigned depth, RefKeys *keys);


/* EN: Extract the fields of a ref ADT as a value-list schema.
 * RU: Получить схему списка полей объекта ref ADT. */
int styxmanaged_result_ref_fields(const char *shape, char fields[STYX_SHAPE_MAX + 1])
{
    const char *end = shape && *shape == 'r' ? styxshape_end(shape) : NULL;
    if (!end || end - shape <= 3 || end - shape > STYX_SHAPE_MAX)
    {
        return 0;
    }
    size_t n = (size_t)(end - shape) - 3;
    fields[0] = '(';
    memcpy(fields + 1, shape + 2, n);
    fields[n + 1] = ')';
    fields[n + 2] = 0;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_ref_fields(const char *shape, char fields[STYX_SHAPE_MAX + 1])
{
    return styxmanaged_result_ref_fields(shape, fields) && !strchr(fields, 'r');
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
        if (!first)
        {
            return 0;
        }
        size_t length = (size_t)(styxshape_end(first) - first);
        return value->size == 1 && (size_t)(styxshape_end(shape) - shape) == length &&
            !memcmp(shape, first, length);
    }
    char fields[STYX_SHAPE_MAX + 1];
    if (index != keys->count || index >= (keys->limit ? keys->limit : STYX_CALL_LEGACY_REF_KEYS) ||
        !styxmanaged_result_ref_fields(shape, fields) || (!keys->nested && strchr(fields, 'r')))
    {
        return 0;
    }
    keys->type[keys->count++] = NULL;
    if (!managed_children(shape + 2, value->payload + 1, value->size - 1, 0, depth + 1, keys))
    {
        return 0;
    }
    keys->type[index] = shape;
    if (keys->definitions)
    {
        keys->definitions[index] = (StyxRefDefinition){shape, *value};
    }
    return 1;
}
