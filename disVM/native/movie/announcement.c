/* Copyright (c) 2026 bhgv
 * EN: Existing S-record codecs encode a scoped logical service generation, never a network locator.
 * RU: S-record codecs кодируют scoped logical generation, не network locator.
 */
#include "announcement.h"
#include <string.h>


/* EN/RU: See announcement.h; no state, heap or transport is retained.
 * См. announcement.h; state/heap/transport не удерживаются. */
int movie_offer_binding(unsigned long tag, void *out, size_t capacity, size_t *size)
{
    FVM_SBinding binding;
    unsigned char payload[128];
    size_t length;
    memset(&binding, 0, sizeof(binding));
    binding.tag = tag;
    binding.version = MOVIE_OFFER_VERSION;
    binding.type_size = sizeof(MOVIE_OFFER_TYPE)-1;
    memcpy(binding.type, MOVIE_OFFER_TYPE, sizeof(MOVIE_OFFER_TYPE));
    return tag && fvm_sbindings_payload_write(&binding, 1, payload, sizeof(payload), &length) == FVM_S_OK &&
        fvm_srecord_write(0, payload, length, out, capacity, size) == FVM_S_OK;
}


/* EN/RU: See announcement.h; canonical integer is shared with all existing protocol implementations.
 * См. announcement.h; canonical integer общий с существующими protocol implementations. */
int movie_offer_record(unsigned long tag, unsigned state, unsigned long long generation,
    void *out, size_t capacity, size_t *size)
{
    unsigned char payload[1+FVM_SRECORD_INTEGER_MAX];
    FVM_SInteger value;
    size_t count;
    if (!tag || !generation || generation > MOVIE_OFFER_GENERATION_MAX || (state != MOVIE_OFFER_READY && state != MOVIE_OFFER_CLOSED))
    {
        return 0;
    }
    payload[0] = (unsigned char)state;
    value.negative = 0;
    value.low = (unsigned long)(generation & FVM_SRECORD_U32);
    value.high = (unsigned long)(generation >> 32);
    return fvm_sinteger_write(&value, payload+1, sizeof(payload)-1, &count) == FVM_S_OK &&
        fvm_srecord_write(tag, payload, count+1, out, capacity, size) == FVM_S_OK;
}
