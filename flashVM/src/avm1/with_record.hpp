/* FlashVM: bounded decoding for an ActionWith header and its nested action body. */
#ifndef FVM_AVM1_WITH_RECORD_HPP
#define FVM_AVM1_WITH_RECORD_HPP /* Private With record decoder guard. */
#include <stddef.h>
#include "encoding_constants.h"
/* Decode external-body and historical inline-body forms without reading outside the record. */
inline bool fvm_with_record(const unsigned char *data, size_t size, size_t start, size_t length,
                            size_t &body, size_t &count, size_t &next)
{
    if (start > size || length > size - start || length < FVM_AVM1_WITH_HEADER_BYTES)
        return false;
    count = data[start] | ((size_t)data[start + 1] << 8);
    body = start + FVM_AVM1_WITH_HEADER_BYTES;
    if ((length != FVM_AVM1_WITH_HEADER_BYTES && length - FVM_AVM1_WITH_HEADER_BYTES != count) ||
        count > size - body)
        return false;
    next = body + count;
    return true;
}
#endif
