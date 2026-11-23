/* Copyright (c) 2026 bhgv
 * EN: Validate the request and the two linked positions of a graph reply.
 * RU: Проверка запроса и двух связанных частей ответа с графом ссылок.
 */

/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_graph_positions(const char *shape, const FVM_SRecord *arguments, StyxGraphPositions *positions)
{
    memset(positions, 0, sizeof(*positions));
    if (arguments->tag != STYX_MANAGED_POSITION_GRAPH_TAG && arguments->tag != STYX_MANAGED_POSITION_FLAT_TAG)
    {
        return 1;
    }
    size_t offset = 0, position = 0;
    for (const char *type = shape + 1; *type != ')'; position++)
    {
        FVM_SRecord record;
        size_t used;
        const char *next = styxshape_end(type);
        if (!next || position >= STYX_SHAPE_MAX || offset >= arguments->size ||
            fvm_srecord_read(arguments->payload + offset, arguments->size - offset,
                STYX_MANAGED_LIST_LIMIT, &record, &used) != FVM_S_OK)
        {
            return 0;
        }
        if ((*type == 'r' || *type == 'h') && record.tag == STYX_MANAGED_REF_TAG && record.size)
        {
            unsigned index = record.payload[0];
            if (!positions->argument[index])
            {
                positions->argument[index] = (unsigned short)(position + 1);
                positions->reference[position] = (unsigned short)(index + 1);
            }
        }
        type = next;
        offset += used;
    }
    return offset == arguments->size;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_graph_update_index(const StyxGraphPositions *positions, const FVM_SRecord *update,
    size_t originals, size_t *index)
{
    if (update->size < 2)
    {
        return 0;
    }
    unsigned value = update->payload[0];
    if (update->tag == STYX_MANAGED_ARG_UPDATE_TAG)
    {
        if (value >= STYX_SHAPE_MAX || !positions->reference[value])
        {
            return 0;
        }
        value = positions->reference[value] - 1;
    }
    else if (update->tag != STYX_MANAGED_REF_TAG || positions->argument[value])
    {
        return 0;
    }
    if (value >= originals)
    {
        return 0;
    }
    *index = value;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
unsigned long styxmanaged_graph_updates_tag(unsigned long request)
{
    if (request == STYX_MANAGED_POSITION_GRAPH_TAG)
    {
        return STYX_MANAGED_POSITION_UPDATES_TAG;
    }
    return request == STYX_MANAGED_SPARSE_GRAPH_TAG ?
        STYX_MANAGED_SPARSE_GRAPH_UPDATES_TAG : STYX_MANAGED_GRAPH_UPDATES_TAG;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_request_payload(const char *shape, const FVM_SRecord *list)
{
    if (!list)
    {
        return 0;
    }
    RefKeys keys = {0};
    if (styxmanaged_graph_tag(list->tag))
    {
        keys.nested = 1;
        keys.limit = STYX_CALL_REF_KEYS;
    }
    return managed_payload(shape, list->payload, list->size, &keys);
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_channel_payload(const char *shape, const FVM_SRecord *list,
    int (*admit)(const void *, const char *, uint32_t), const void *context)
{
    return admit && styxmanaged_channel_value(shape, list, admit, context, 0);
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_channel_value(const char *shape, const FVM_SRecord *list,
    int (*admit)(const void *, const char *, uint32_t), const void *context, int grants)
{
    RefKeys keys = {.nested = 1, .limit = STYX_CALL_REF_KEYS, .channel = admit,
        .channel_context = context, .message_grants = grants};
    return list && list->tag == STYX_MANAGED_GRAPH_TAG &&
        managed_payload(shape, list->payload, list->size, &keys);
}

/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_graph_reply(const StyxCallShape *shape, const FVM_SRecord *arguments,
    const StyxReplyField fields[2])
{
    RefKeys keys = {.nested = 1, .limit = STYX_CALL_REF_KEYS};
    FVM_SRecord result, updates;
    size_t used;
    if (!managed_payload(shape->arguments, arguments->payload, arguments->size, &keys) ||
        fvm_srecord_read(fields[0].bytes, fields[0].size, STYX_MANAGED_LIST_LIMIT, &result, &used) != FVM_S_OK ||
        used != fields[0].size)
    {
        return 0;
    }
    size_t originals = keys.count;
    int sparse = styxmanaged_sparse_graph_tag(arguments->tag);
    unsigned long update_tag = styxmanaged_graph_updates_tag(arguments->tag);
    StyxGraphPositions positions;
    if (!styxmanaged_graph_positions(shape->arguments, arguments, &positions))
    {
        return 0;
    }
    keys.exported_channels = 1;
    if ((!strcmp(shape->result, "()") ? result.tag != STYX_MANAGED_NIL_TAG || result.size :
            result.tag != STYX_DYNAMIC_REPLY_TAG ||
            !managed_payload(shape->result, result.payload, result.size, &keys)) ||
        fvm_srecord_read(fields[1].bytes, fields[1].size, STYX_MANAGED_LIST_LIMIT, &updates, &used) != FVM_S_OK ||
        used != fields[1].size || updates.tag != update_tag)
    {
        return 0;
    }
    keys.exported_channels = 0;
    size_t next = 0;
    for (size_t offset = 0; offset < updates.size; offset += used)
    {
        FVM_SRecord update;
        size_t index;
        if (fvm_srecord_read(updates.payload + offset, updates.size - offset, STYX_MANAGED_LIST_LIMIT,
                &update, &used) != FVM_S_OK ||
            !styxmanaged_graph_update_index(&positions, &update, originals, &index))
        {
            return 0;
        }
        if (index < next)
        {
            return 0;
        }
        StyxFieldDelta change;
        if (*keys.type[index] == 'h' ?
            arguments->tag != STYX_MANAGED_POSITION_GRAPH_TAG ||
                !styxmanaged_table_visit(update.payload + 1, update.size - 1, 1, NULL, NULL) :
            sparse ? !styxmanaged_graph_fields(keys.type[index], &update, &change) ||
                !managed_payload(change.schema, change.fields.payload, change.fields.size, &keys) :
            !managed_children(keys.type[index] + 2, update.payload + 1, update.size - 1, 0, 0, &keys))
        {
            return 0;
        }
        next = index + 1;
    }
    return 1;
}

/* EN: Only tuples introduce more native lanes. Array elements compare within one lane.
 * RU: Только кортежи добавляют слоты; элементы массива сравниваются внутри одного слота. */
static int graph_compare(const FVM_SRecord *left, const FVM_SRecord *right,
    unsigned *mask, unsigned *lane);

static int graph_sequence(const FVM_SRecord *left, const FVM_SRecord *right,
    unsigned *mask, unsigned *lane)
{
    size_t l = 0, r = 0;
    while (l < left->size && r < right->size)
    {
        FVM_SRecord a, b;
        size_t na, nb;
        if (fvm_srecord_read(left->payload + l, left->size - l, STYX_MANAGED_LIST_LIMIT, &a, &na) != FVM_S_OK ||
            fvm_srecord_read(right->payload + r, right->size - r, STYX_MANAGED_LIST_LIMIT, &b, &nb) != FVM_S_OK ||
            !graph_compare(&a, &b, mask, lane))
        {
            return 0;
        }
        l += na;
        r += nb;
    }
    return l == left->size && r == right->size;
}

static int graph_compare(const FVM_SRecord *left, const FVM_SRecord *right,
    unsigned *mask, unsigned *lane)
{
    if (left->tag == right->tag && left->tag == STYX_CALL_TUPLE_TAG)
    {
        return graph_sequence(left, right, mask, lane);
    }
    int equal = left->tag == right->tag && (left->tag == STYX_MANAGED_REF_TAG ?
        left->size && right->size && left->payload[0] == right->payload[0] :
        left->tag == STYX_MANAGED_ARRAY_TAG ? graph_sequence(left, right, NULL, NULL) :
        left->size == right->size && !memcmp(left->payload, right->payload, left->size));
    if (!mask)
    {
        return equal;
    }
    if (*lane >= STYX_CALL_VALUES)
    {
        return 0;
    }
    if (!equal)
    {
        *mask |= 1U << *lane;
    }
    ++*lane;
    return 1;
}

int styxmanaged_graph_mask(const FVM_SRecord *before, const FVM_SRecord *after, unsigned *mask)
{
    if (!before->size || !after->size)
    {
        return 0;
    }
    FVM_SRecord left = {.payload = before->payload + 1, .size = before->size - 1};
    FVM_SRecord right = {.payload = after->payload + 1, .size = after->size - 1};
    unsigned lane = 0;
    *mask = 0;
    return graph_sequence(&left, &right, mask, &lane);
}

/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_graph_diff(const char *type, const FVM_SRecord *before, const FVM_SRecord *after,
    void *output, size_t capacity, size_t *used)
{
    char schema[STYX_SHAPE_MAX + 1];
    FieldLeaves left = {0}, right = {0};
    if (!before->size || !after->size || !styxmanaged_result_ref_fields(type, schema))
    {
        return 0;
    }
    FVM_SRecord a = {.payload = before->payload + 1, .size = before->size - 1};
    FVM_SRecord b = {.payload = after->payload + 1, .size = after->size - 1};
    if (!field_leaves(schema + 1, &a, &left) || !field_leaves(schema + 1, &b, &right))
    {
        return 0;
    }
    unsigned char *bytes = output;
    unsigned mask = 0;
    size_t size = 1;
    for (size_t i = 0; i < right.count; i++)
    {
        const FVM_SRecord *value = &right.node[i];
        if (graph_compare(&left.node[i], value, NULL, NULL))
        {
            continue;
        }
        size_t n;
        if (size >= capacity || fvm_srecord_write(value->tag, value->payload, value->size,
            bytes + size, capacity - size, &n) != FVM_S_OK)
        {
            return 0;
        }
        mask |= 1U << i;
        size += n;
    }
    if (mask)
    {
        bytes[0] = (unsigned char)mask;
    }
    *used = mask ? size : 0;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_graph_fields(const char *type, const FVM_SRecord *update, StyxFieldDelta *change)
{
    char schema[STYX_SHAPE_MAX + 1];
    FieldLeaves leaves = {0};
    if ((update->tag != STYX_MANAGED_REF_TAG && update->tag != STYX_MANAGED_ARG_UPDATE_TAG) || update->size < 3 ||
        !styxmanaged_result_ref_fields(type, schema) || !field_leaves(schema + 1, NULL, &leaves))
    {
        return 0;
    }
    unsigned mask = update->payload[1];
    if (!mask || (mask >> leaves.count))
    {
        return 0;
    }
    select_lanes(&leaves, mask, change->schema);
    change->object = update->payload[0];
    change->mask = mask;
    change->fields = *update;
    change->fields.payload += 2;
    change->fields.size -= 2;
    return 1;
}
