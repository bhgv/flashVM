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
int styxmanaged_ref_definitions(const char *shape, const FVM_SRecord *list, int nested,
    StyxRefDefinition *definitions, size_t limit, size_t *count)
{
    RefKeys keys = {.definitions = definitions, .limit = limit, .nested = nested};
    if (!list || !definitions || !count || !limit || limit > STYX_CALL_REF_KEYS ||
        !managed_payload(shape, list->payload, list->size, &keys))
    {
        return 0;
    }
    *count = keys.count;
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_payload_linked(const char *shape, const void *bytes, size_t size,
    const char *argument_shape, const FVM_SRecord *arguments)
{
    RefKeys keys = {0};
    if (!arguments || !managed_payload(argument_shape, arguments->payload, arguments->size, &keys))
    {
        return 0;
    }
    keys.nested = 1;
    keys.limit = arguments->tag == STYX_MANAGED_WIDE_REFS_TAG ? STYX_CALL_REF_KEYS : STYX_CALL_LEGACY_REF_KEYS;
    return managed_payload(shape, bytes, size, &keys);
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


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxmanaged_result_refs(const char *shape)
{
    for (const char *ref = strchr(shape, 'r'); ref; ref = strchr(ref + 1, 'r'))
    {
        char fields[STYX_SHAPE_MAX + 1];
        StyxScalarLayout layout;
        if (!styxmanaged_result_ref_fields(ref, fields) ||
            !styxmanaged_layout(fields, &layout) || layout.size > 64)
        {
            return 0;
        }
    }
    return 1;
}

#include "managed_graph_validation.inc"
