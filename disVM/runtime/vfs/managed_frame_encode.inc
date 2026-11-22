/* Copyright (c) 2026 bhgv
 * EN: Encode checked frame values and form their enclosing records.
 * RU: Кодирование проверенных значений кадра и формирование внешних записей.
 */


/* EN: Encode one laid-out frame with optional argument identities in its result.
 * RU: Кодировать кадр с возможностью связать ссылки результата с аргументами. */
static int encode_frame(FrameEncode *encode, unsigned long tag, const char *shape,
    size_t slot_size, void *bytes, size_t capacity, size_t *used,
    const char *argument_shape, const FVM_SRecord *arguments)
{
    if (!styxmanaged_layout(shape, &encode->layout) || (encode->layout.size && !encode->slots) ||
        encode->layout.size > slot_size || !bytes || !used || !capacity)
    {
        return 0;
    }
    encode->capacity = capacity < STYX_MANAGED_LIST_LIMIT ? capacity : STYX_MANAGED_LIST_LIMIT;
    encode->scratch = malloc(DVM_STYX_TUPLE_SCRATCH * encode->capacity);
    if (!encode->scratch)
    {
        return 0;
    }
    size_t size;
    int ok = encode_nodes(encode, shape + 1, 0, &size) && encode->leaf == encode->layout.count &&
        (arguments ? styxmanaged_payload_linked(shape, encode->scratch, size, argument_shape, arguments) :
            encode->refs->result || styxmanaged_payload_valid(shape, encode->scratch, size)) &&
        fvm_srecord_write(tag, encode->scratch, size, bytes, encode->capacity, used) == FVM_S_OK;
    free(encode->scratch);
    return ok;
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_value_frame_write(unsigned long tag, const char *shape, const void *slots,
    size_t slot_size, void *bytes, size_t capacity, size_t *used)
{
    DVM_StyxRefTable refs = {0};
    FrameEncode encode = {.slots = slots, .refs = &refs};
    return encode_frame(&encode, tag, shape, slot_size, bytes, capacity, used, NULL, NULL);
}
