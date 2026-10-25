/* Fixed-size recovery boundaries for batches of independently checked binary S-records. */
#include "flashvm/sslot.h"
#include <string.h>


/* The ordinary writer preflights before touching output; padding cannot subsequently fail. */
FVM_SResult fvm_sslot_write_sized(
    unsigned long tag, const void *payload, size_t size, size_t slot_bytes, void *output, size_t capacity, size_t *used)
{
    size_t n;
    FVM_SResult result;
    if (!output || !used || (!payload && size) || slot_bytes < FVM_SSLOT_MIN_BYTES || slot_bytes > FVM_SSLOT_MAX_BYTES)
    {
        return FVM_S_INVALID;
    }
    if (capacity < slot_bytes)
    {
        return FVM_S_LIMIT;
    }
    result = fvm_srecord_write(tag, payload, size, (unsigned char *)output, slot_bytes, &n);
    if (result != FVM_S_OK)
    {
        return result;
    }
    memset((unsigned char *)output + n, 0, slot_bytes - n);
    *used = slot_bytes;
    return FVM_S_OK;
}


/* Preserve the original profile-1 convenience call without making it a channel default. */
FVM_SResult
fvm_sslot_write(unsigned long tag, const void *payload, size_t size, void *output, size_t capacity, size_t *used)
{
    return fvm_sslot_write_sized(tag, payload, size, FVM_SSLOT_BYTES, output, capacity, used);
}


/* Never let corrupted embedded framing cross the externally fixed boundary. */
FVM_SResult fvm_sslot_read_sized(const void *input, size_t size, size_t slot_bytes, FVM_SRecord *view, size_t *used)
{
    FVM_SRecord record;
    FVM_SResult result;
    const unsigned char *bytes = (const unsigned char *)input;
    size_t n, i;
    if ((!input && size) || !view || !used || slot_bytes < FVM_SSLOT_MIN_BYTES || slot_bytes > FVM_SSLOT_MAX_BYTES)
    {
        return FVM_S_INVALID;
    }
    if (size < slot_bytes)
    {
        return FVM_S_MORE;
    }
    if (size != slot_bytes)
    {
        return FVM_S_INVALID;
    }
    result = fvm_srecord_read(input, size, slot_bytes, &record, &n);
    if (result != FVM_S_OK)
    {
        return result == FVM_S_MORE ? FVM_S_INVALID : result;
    }
    for (i = n; i < size; ++i)
    {
        if (bytes[i])
        {
            return FVM_S_INVALID;
        }
    }
    *view = record;
    *used = size;
    return FVM_S_OK;
}


/* Default-size reader remains available to fixed profile-1 users. */
FVM_SResult fvm_sslot_read(const void *input, size_t size, FVM_SRecord *view, size_t *used)
{
    return fvm_sslot_read_sized(input, size, FVM_SSLOT_BYTES, view, used);
}


/* Corrupt slots are reported independently; even damaged lengths cannot redirect iteration. */
FVM_SResult fvm_sslot_batch_sized(
    const void *input, size_t size, size_t slot_bytes, FVM_SSlotNotify notify, void *userdata, FVM_SSlotReport *report)
{
    FVM_SSlotReport result;
    FVM_SRecord record;
    FVM_SResult status;
    size_t i, used;
    const unsigned char *bytes = (const unsigned char *)input;
    if ((!input && size) || !report || slot_bytes < FVM_SSLOT_MIN_BYTES || slot_bytes > FVM_SSLOT_MAX_BYTES ||
        size % slot_bytes)
    {
        return FVM_S_INVALID;
    }
    result.slots = size / slot_bytes;
    if (result.slots > FVM_SSLOT_BATCH)
    {
        return FVM_S_LIMIT;
    }
    result.valid = result.damaged = 0;
    for (i = 0; i < result.slots; ++i)
    {
        status = fvm_sslot_read_sized(bytes + i * slot_bytes, slot_bytes, slot_bytes, &record, &used);
        if (status == FVM_S_OK)
        {
            ++result.valid;
        }
        else
        {
            ++result.damaged;
        }
        if (notify)
        {
            notify(i, status, status == FVM_S_OK ? &record : 0, userdata);
        }
    }
    *report = result;
    return FVM_S_OK;
}


/* Profile-1 batch wrapper uses the same independently bounded traversal. */
FVM_SResult
fvm_sslot_batch(const void *input, size_t size, FVM_SSlotNotify notify, void *userdata, FVM_SSlotReport *report)
{
    return fvm_sslot_batch_sized(input, size, FVM_SSLOT_BYTES, notify, userdata, report);
}
