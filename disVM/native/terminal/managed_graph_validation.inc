/* Copyright (c) 2026 bhgv
 * EN: Validate the request and the two linked positions of a graph reply.
 * RU: Проверка запроса и двух связанных частей ответа с графом ссылок.
 */

int styxmanaged_request_payload(const char *shape, const FVM_SRecord *list)
{
    if (!list)
    {
        return 0;
    }
    RefKeys keys = {0};
    if (list->tag == STYX_MANAGED_GRAPH_TAG)
    {
        keys.nested = 1;
        keys.limit = STYX_CALL_REF_KEYS;
    }
    return managed_payload(shape, list->payload, list->size, &keys);
}

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
    if ((!strcmp(shape->result, "()") ? result.tag != STYX_MANAGED_NIL_TAG || result.size :
            result.tag != STYX_DYNAMIC_REPLY_TAG ||
            !managed_payload(shape->result, result.payload, result.size, &keys)) ||
        fvm_srecord_read(fields[1].bytes, fields[1].size, STYX_MANAGED_LIST_LIMIT, &updates, &used) != FVM_S_OK ||
        used != fields[1].size || updates.tag != STYX_MANAGED_GRAPH_UPDATES_TAG)
    {
        return 0;
    }
    size_t next = 0;
    for (size_t offset = 0; offset < updates.size; offset += used)
    {
        FVM_SRecord update;
        if (fvm_srecord_read(updates.payload + offset, updates.size - offset, STYX_MANAGED_LIST_LIMIT,
                &update, &used) != FVM_S_OK || update.tag != STYX_MANAGED_REF_TAG || update.size < 2)
        {
            return 0;
        }
        size_t index = update.payload[0];
        if (index < next || index >= originals ||
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
