/* Fixed-size recovery boundaries for batches of independently checked binary S-records. */
#include "flashvm/sslot.h"
#include <string.h>
/* Stage the complete padded slot so all error paths preserve caller output. */
FVM_SResult fvm_sslot_write(unsigned long tag, const void *payload, size_t size, void *output,
                            size_t capacity, size_t *used)
{
    unsigned char bytes[FVM_SSLOT_BYTES];
    size_t n;
    FVM_SResult result;
    if (!output || !used || (!payload && size))
        return FVM_S_INVALID;
    if (capacity < FVM_SSLOT_BYTES)
        return FVM_S_LIMIT;
    result = fvm_srecord_write(tag, payload, size, bytes, sizeof(bytes), &n);
    if (result != FVM_S_OK)
        return result;
    memset(bytes + n, 0, sizeof(bytes) - n);
    memcpy(output, bytes, sizeof(bytes));
    *used = sizeof(bytes);
    return FVM_S_OK;
}
/* Never let corrupted embedded framing cross the externally fixed boundary. */
FVM_SResult fvm_sslot_read(const void *input, size_t size, FVM_SRecord *view, size_t *used)
{
    FVM_SRecord record;
    FVM_SResult result;
    const unsigned char *bytes = (const unsigned char *)input;
    size_t n, i;
    if ((!input && size) || !view || !used)
        return FVM_S_INVALID;
    if (size < FVM_SSLOT_BYTES)
        return FVM_S_MORE;
    if (size != FVM_SSLOT_BYTES)
        return FVM_S_INVALID;
    result = fvm_srecord_read(input, size, FVM_SSLOT_BYTES, &record, &n);
    if (result != FVM_S_OK)
        return result == FVM_S_MORE ? FVM_S_INVALID : result;
    for (i = n; i < size; ++i)
        if (bytes[i])
            return FVM_S_INVALID;
    *view = record;
    *used = size;
    return FVM_S_OK;
}
/* Corrupt slots are reported independently; even damaged lengths cannot redirect iteration. */
FVM_SResult fvm_sslot_batch(const void *input, size_t size, FVM_SSlotNotify notify, void *userdata,
                            FVM_SSlotReport *report)
{
    FVM_SSlotReport result;
    FVM_SRecord record;
    FVM_SResult status;
    size_t i, used;
    const unsigned char *bytes = (const unsigned char *)input;
    if ((!input && size) || !report || size % FVM_SSLOT_BYTES)
        return FVM_S_INVALID;
    result.slots = size / FVM_SSLOT_BYTES;
    if (result.slots > FVM_SSLOT_BATCH)
        return FVM_S_LIMIT;
    result.valid = result.damaged = 0;
    for (i = 0; i < result.slots; ++i)
    {
        status = fvm_sslot_read(bytes + i * FVM_SSLOT_BYTES, FVM_SSLOT_BYTES, &record, &used);
        if (status == FVM_S_OK)
            ++result.valid;
        else
            ++result.damaged;
        if (notify)
            notify(i, status, status == FVM_S_OK ? &record : 0, userdata);
    }
    *report = result;
    return FVM_S_OK;
}
