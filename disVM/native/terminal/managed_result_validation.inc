/* Copyright (c) 2026 bhgv
 * EN: Share identities between a managed request and its result record.
 * RU: Общая идентичность ссылок между управляемым запросом и его результатом.
 */


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_payload_valid(const char *shape, const void *bytes, size_t size)
{
    RefKeys keys = {0};
    return managed_payload(shape, bytes, size, &keys);
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_payload_linked(const char *shape, const void *bytes, size_t size,
    const char *argument_shape, const FVM_SRecord *arguments)
{
    RefKeys keys = {0};
    return arguments && managed_payload(argument_shape, arguments->payload, arguments->size, &keys) &&
        managed_payload(shape, bytes, size, &keys);
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_result_read(const char *shape, const void *bytes, size_t size,
    const char *argument_shape, const FVM_SRecord *arguments, FVM_SRecord *result)
{
    FVM_SRecord decoded;
    size_t used;
    if (!result || size > STYX_MANAGED_LIST_LIMIT ||
        fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &decoded, &used) != FVM_S_OK ||
        used != size || decoded.tag != STYX_DYNAMIC_REPLY_TAG ||
        !styxmanaged_payload_linked(shape, decoded.payload, decoded.size, argument_shape, arguments))
    {
        return 0;
    }
    *result = decoded;
    return 1;
}
