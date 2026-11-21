/* Copyright (c) 2026 bhgv
 * EN: Map compiled exports to the mounted gateway and verify exact load offers.
 * RU: Сопоставление скомпилированных экспортов со шлюзом и проверка предложений load.
 */
#include "load_profile.h"
#include "scalar_verify.h"
#include "flashvm/srecord.h"
#include <string.h>


/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxloadedrequest(uint32_t signature)
{
    switch (signature)
    {
    case DVM_SCALAR_INT_SIGNATURE:
        return STYX_FUNCTION_REQUEST_TAG;
    case DVM_SCALAR_BIG_SIGNATURE:
        return STYX_FUNCTION_BIG_TAG;
    case DVM_TUPLE_SIGNATURE:
        return STYX_TUPLE_REQUEST_TAG;
    case DVM_REF_SIGNATURE:
        return STYX_REF_REQUEST_TAG;
    case DVM_CALL_LIST_SIGNATURE:
        return STYX_CALL_LIST_REQUEST_TAG;
    case DVM_REF_PAIR_SIGNATURE:
        return STYX_REF_PAIR_REQUEST_TAG;
    default:
        return 0;
    }
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxloadedoffer(const char *name, uint32_t signature, void *bytes, size_t capacity, size_t *used)
{
    size_t length = styxscalar_name_length(name);
    unsigned char head[INTERFACE_HEADER];
    styxscalar_interface_header(head, signature);
    return length && styxloadedrequest(signature) &&
        fvm_srecord_write_pair(STYX_FUNCTION_PROFILE_TAG, head, sizeof(head), name, length,
            bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxloadedaccept(const char *name, const void *bytes, size_t size)
{
    size_t length = styxscalar_name_length(name), used;
    FVM_SRecord record;
    if (!length ||
        fvm_srecord_read(bytes, size, STYX_SCALAR_INTERFACE_CAPACITY, &record, &used) != FVM_S_OK ||
        used != size || record.tag != STYX_FUNCTION_PROFILE_TAG || record.size != INTERFACE_HEADER + length ||
        record.payload[0] != INTERFACE_KIND || memcmp(record.payload + INTERFACE_HEADER, name, length))
    {
        return 0;
    }
    uint32_t signature = 0;
    for (size_t i = 1; i < INTERFACE_HEADER; i++)
    {
        signature = (signature << 8) | record.payload[i];
    }
    return styxloadedrequest(signature);
}
