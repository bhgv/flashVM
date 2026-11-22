/* Copyright (c) 2026 bhgv
 * EN: Encode and decode result references relative to the original call.
 * RU: Кодирование и декодирование ссылок результата относительно исходного вызова.
 */


/* EN: Match request reference indices to borrowed objects from the mapped call frame.
 * RU: Связать индексы ссылок запроса с заимствованными объектами из кадра вызова. */
static int seed_result(DVM_StyxRefTable *table, const char *shape, const void *slots,
    const FVM_SRecord *arguments)
{
    char refs[STYX_SHAPE_MAX + 1];
    void *objects[STYX_CALL_VALUES] = {0};
    FVM_SRecord records[STYX_CALL_VALUES];
    size_t count;
    if (!arguments || !dvm_styx_ref_slots(shape, slots, refs, objects) ||
        !styxmanaged_ref_records(shape, arguments, records, &count))
    {
        return 0;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (records[i].tag == STYX_MANAGED_NIL_TAG)
        {
            if (objects[i] != H)
            {
                return 0;
            }
            continue;
        }
        if (!objects[i] || objects[i] == H || records[i].tag != STYX_MANAGED_REF_TAG || !records[i].size)
        {
            return 0;
        }
        size_t index = records[i].payload[0];
        if (index == table->count && index < STYX_CALL_VALUES && records[i].size > 1)
        {
            table->objects[table->count++] = objects[i];
        }
        else if (index >= table->count || table->objects[index] != objects[i] || records[i].size != 1)
        {
            return 0;
        }
    }
    return 1;
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_ref_result_read(const StyxCallShape *shape, const void *data, size_t size,
    const void *argument_slots, const FVM_SRecord *arguments, DVM_StyxValueFrame *result)
{
    DVM_StyxRefTable refs = {.result = 1,
        .limit = arguments->tag == STYX_MANAGED_WIDE_REFS_TAG ? STYX_CALL_REF_KEYS : STYX_CALL_LEGACY_REF_KEYS};
    FrameDecode decode = {.shape = shape->result, .refs = &refs};
    if (!result || !styxmanaged_layout(shape->result, &decode.pending.layout) ||
        !styxmanaged_result_read(shape->result, data, size, shape->arguments, arguments, &decode.list) ||
        !seed_result(&refs, shape->arguments, argument_slots, arguments))
    {
        return 0;
    }
    if (!kprotect(decode_frame, &decode) || decode.leaf != decode.pending.layout.count)
    {
        dvm_styx_value_frame_clear(&decode.pending);
        return 0;
    }
    *result = decode.pending;
    return 1;
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_ref_result_write(const StyxCallShape *shape, const void *slots, size_t size,
    const void *argument_slots, const FVM_SRecord *arguments, void *data, size_t capacity, size_t *used)
{
    DVM_StyxRefTable refs = {.result = 1,
        .limit = arguments->tag == STYX_MANAGED_WIDE_REFS_TAG ? STYX_CALL_REF_KEYS : STYX_CALL_LEGACY_REF_KEYS};
    FrameEncode encode = {.slots = slots, .refs = &refs};
    if (!seed_result(&refs, shape->arguments, argument_slots, arguments))
    {
        return 0;
    }
    size_t seeded = refs.count;
    int valid = encode_frame(&encode, STYX_DYNAMIC_REPLY_TAG, shape->result, size, data, capacity, used,
        shape->arguments, arguments);
    for (size_t i = seeded; i < refs.count; i++)
    {
        ptrdel(D2H(refs.objects[i]));
        destroy(refs.objects[i]);
    }
    return valid;
}


/* [GC:] EN/RU: See managed_ref.h. / См. managed_ref.h. */
int dvm_styx_result_fields_read(DVM_StyxRefTable *table, const char *shape,
    const void *payload, size_t size, DVM_StyxValueFrame *frame)
{
    FrameDecode decode = {.shape = shape, .list = {.payload = payload, .size = size}, .refs = table};
    if (!styxmanaged_layout(shape, &decode.pending.layout) ||
        !kprotect(decode_frame, &decode) || decode.leaf != decode.pending.layout.count)
    {
        dvm_styx_value_frame_clear(&decode.pending);
        return 0;
    }
    *frame = decode.pending;
    return 1;
}


/* [GC:] EN/RU: See managed_ref.h. / См. managed_ref.h. */
int dvm_styx_result_fields_write(DVM_StyxRefTable *table, const char *shape, const void *slots,
    size_t size, void *bytes, size_t capacity, size_t *used)
{
    FrameEncode encode = {.slots = slots, .refs = table};
    return encode_frame(&encode, STYX_MANAGED_REF_TAG, shape, size, bytes, capacity, used, NULL, NULL);
}
