/* Copyright (c) 2026 bhgv
 * EN: Optional sequence prefix outside exactly one checked original S-record, no generation in data packets.
 * RU: Необязательный префикс номера перед одной проверенной S-записью; без поколения в пакетах данных.
 */
#include "flashvm/sorder.h"
#include <string.h>


/* EN/RU: See sorder.h; validate prefix into temporary, original record writer is atomic before prefix publication.
 * См. sorder.h; prefix в temporary, original writer атомарен до publication prefix.
 */
FVM_SResult fvm_sordered_write(const FVM_SOrdinal *ordinal, unsigned long tag, const void *bytes,
    size_t size, void *output, size_t capacity, size_t *used)
{
    unsigned char prefix[FVM_SORDER_PREFIX_MAX]; /* Temporary exact prefix. */
    size_t header; /* Exact prefix extent. */
    size_t frame; /* Exact original S-record extent. */
    FVM_SResult result; /* Original codec result, no guessed wire status. */
    if (!output || !used)
    {
        return FVM_S_INVALID;
    }
    result = fvm_sordinal_write(ordinal, prefix, sizeof(prefix), &header);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (capacity < header)
    {
        return FVM_S_LIMIT;
    }
    result = fvm_srecord_write(tag, bytes, size, (unsigned char *)output + header, capacity - header, &frame);
    if (result == FVM_S_OK)
    {
        memcpy(output, prefix, header);
        *used = header + frame;
    }
    return result;
}


/* EN/RU: See sorder.h; both prefix and frame must pass before a borrowed combined view is visible.
 * См. sorder.h; prefix и frame должны пройти проверку до publication combined view.
 */
FVM_SResult fvm_sordered_read(const void *input, size_t size, size_t limit, FVM_SOrderedRecord *view, size_t *used)
{
    FVM_SOrderedRecord next; /* Private complete borrowed view. */
    size_t header; /* Exact prefix extent. */
    size_t frame; /* Exact original S-record extent. */
    FVM_SResult result; /* Original codec status. */
    if ((!input && size) || !view || !used || !limit || limit > FVM_SRECORD_BODY_MAX)
    {
        return FVM_S_INVALID;
    }
    result = fvm_sordinal_read(input, size, &next.ordinal, &header);
    if (result != FVM_S_OK)
    {
        return result;
    }
    result = fvm_srecord_read((const unsigned char *)input + header, size - header, limit, &next.record, &frame);
    if (result == FVM_S_OK)
    {
        next.encoded_size = header + frame;
        *view = next;
        *used = next.encoded_size;
    }
    return result;
}
