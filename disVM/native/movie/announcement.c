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


/* EN/RU: See announcement.h; reuse canonical integer codec before publishing native scalar outputs.
 * См. announcement.h; общий canonical integer codec до публикации native scalar outputs.
 */
int movie_offer_parse(const void *input, size_t size, unsigned *state, unsigned long long *generation)
{
    const unsigned char *bytes = input; /* Borrowed exact payload, no native wire struct. */
    FVM_SInteger number; /* Checked native limbs from the shared codec. */
    size_t consumed; /* Exact canonical integer extent. */
    if (!bytes || size < 2 || !state || !generation ||
        (bytes[0] != MOVIE_OFFER_READY && bytes[0] != MOVIE_OFFER_CLOSED) ||
        fvm_sinteger_read(bytes + 1, size - 1, &number, &consumed) != FVM_S_OK ||
        consumed != size - 1 || number.negative)
    {
        return 0;
    }
    unsigned long long value = ((unsigned long long)number.high << 32) | number.low; /* Checked positive generation. */
    if (!value || value > MOVIE_OFFER_GENERATION_MAX)
    {
        return 0;
    }
    *state = bytes[0];
    *generation = value;
    return 1;
}
