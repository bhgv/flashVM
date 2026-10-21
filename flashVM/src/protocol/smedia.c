/* Original bounded object reassembly over fixed checked slots; transport and codecs stay separate.
 */
#include "flashvm/smedia.h"
#include <string.h>
/* Validate portable metadata before arithmetic, buffer access or session adoption. */
static int media_info(const FVM_SMediaInfo *info)
{
    return info && (info->mode == FVM_SMEDIA_FILE || info->mode == FVM_SMEDIA_LIVE) &&
           info->stream && info->stream <= FVM_SRECORD_U32 && info->epoch &&
           info->epoch <= FVM_SRECORD_U32 && info->object && info->object <= FVM_SRECORD_U32 &&
           info->time_ms <= FVM_SRECORD_U32 && info->total && info->total <= FVM_SMEDIA_MAX_BYTES;
}
/* Ceiling division is safe after the strict object-size validation. */
static unsigned long media_parts(const FVM_SMediaInfo *info)
{
    return (info->total - 1) / FVM_SMEDIA_PAYLOAD + 1;
}
/* A final fragment has an exact short extent; no overlapping or sparse peer-selected offsets. */
static size_t media_size(const FVM_SMediaInfo *info, unsigned long part)
{
    unsigned long remaining = info->total - part * FVM_SMEDIA_PAYLOAD;
    return (size_t)(remaining < FVM_SMEDIA_PAYLOAD ? remaining : FVM_SMEDIA_PAYLOAD);
}
/* Append one canonical nonnegative Uint32 without native-endian field stores. */
static size_t media_put(unsigned char *out, unsigned long value)
{
    FVM_SInteger number;
    size_t used = 0;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}
/* Consume an exact canonical Uint32, leaving incomplete bodies unpublished. */
static int media_get(const unsigned char **p, size_t *left, unsigned long *value)
{
    FVM_SInteger number;
    size_t used;
    if (fvm_sinteger_read(*p, *left, &number, &used) != FVM_S_OK || number.high || number.negative)
        return 0;
    *p += used;
    *left -= used;
    *value = number.low;
    return 1;
}
/* Encode a fragment in local storage before the slot writer commits any destination bytes. */
FVM_SResult fvm_smedia_write(unsigned long tag, const FVM_SMediaInfo *info, unsigned long part,
                             const void *data, size_t size, void *output, size_t capacity,
                             size_t *used)
{
    unsigned char payload[FVM_SSLOT_BYTES];
    size_t n = 0;
    if (!tag || tag > FVM_SRECORD_U32 || !media_info(info) || !data || part >= media_parts(info) ||
        size != media_size(info, part))
        return FVM_S_INVALID;
    n += media_put(payload + n, FVM_SMEDIA_VERSION);
    n += media_put(payload + n, info->mode);
    n += media_put(payload + n, info->stream);
    n += media_put(payload + n, info->epoch);
    n += media_put(payload + n, info->object);
    n += media_put(payload + n, info->time_ms);
    n += media_put(payload + n, info->total);
    n += media_put(payload + n, part);
    memcpy(payload + n, data, size);
    return fvm_sslot_write(tag, payload, n + size, output, capacity, used);
}
/* Decode without accepting tag declarations from a lossy/untrusted media datagram. */
FVM_SResult fvm_smedia_read(const void *input, size_t size, unsigned long tag, FVM_SMediaPiece *out)
{
    FVM_SRecord record;
    FVM_SMediaPiece piece;
    FVM_SResult result;
    unsigned long version;
    const unsigned char *p;
    size_t used, left;
    if (!out || !tag || tag > FVM_SRECORD_U32)
        return FVM_S_INVALID;
    result = fvm_sslot_read(input, size, &record, &used);
    if (result != FVM_S_OK)
        return result;
    if (record.tag != tag)
        return FVM_S_UNKNOWN;
    memset(&piece, 0, sizeof(piece));
    p = record.payload;
    left = record.size;
    if (!media_get(&p, &left, &version) || version != FVM_SMEDIA_VERSION ||
        !media_get(&p, &left, &piece.info.mode) || !media_get(&p, &left, &piece.info.stream) ||
        !media_get(&p, &left, &piece.info.epoch) || !media_get(&p, &left, &piece.info.object) ||
        !media_get(&p, &left, &piece.info.time_ms) || !media_get(&p, &left, &piece.info.total) ||
        !media_get(&p, &left, &piece.part) || !media_info(&piece.info) ||
        piece.part >= media_parts(&piece.info) || left != media_size(&piece.info, piece.part))
        return FVM_S_INVALID;
    piece.data = p;
    piece.size = left;
    *out = piece;
    return FVM_S_OK;
}
/* Validate the receiver's native storage contract; callers must not modify private state. */
static int media_receiver(const FVM_SMediaReceiver *r)
{
    return r && r->struct_size == sizeof(*r) && media_info(&r->info) && r->tag &&
           r->tag <= FVM_SRECORD_U32 && r->buffer && r->capacity >= r->info.total &&
           r->received <= media_parts(&r->info);
}
/* Replace identity/bitmap only after all caller-owned buffer and capacity checks pass. */
FVM_SResult fvm_smedia_receiver_init(FVM_SMediaReceiver *out, unsigned long tag,
                                     const FVM_SMediaInfo *info, void *buffer, size_t capacity)
{
    FVM_SMediaReceiver r;
    if (!out || !tag || tag > FVM_SRECORD_U32 || !media_info(info) || !buffer)
        return FVM_S_INVALID;
    if (capacity < info->total)
        return FVM_S_LIMIT;
    memset(&r, 0, sizeof(r));
    r.struct_size = sizeof(r);
    r.info = *info;
    r.tag = tag;
    r.buffer = (unsigned char *)buffer;
    r.capacity = capacity;
    *out = r;
    return FVM_S_OK;
}
/* Compare individual fields, never native struct padding or addresses. */
static int media_same(const FVM_SMediaInfo *a, const FVM_SMediaInfo *b)
{
    return a->mode == b->mode && a->stream == b->stream && a->epoch == b->epoch &&
           a->object == b->object && a->time_ms == b->time_ms && a->total == b->total;
}
/* Duplicate and out-of-order packets cannot overwrite an already accepted distinct fragment. */
FVM_SResult fvm_smedia_accept(FVM_SMediaReceiver *r, const void *input, size_t size, int *complete)
{
    FVM_SMediaPiece piece;
    FVM_SResult result;
    size_t offset, byte;
    unsigned int mask;
    if (!media_receiver(r) || !complete)
        return FVM_S_INVALID;
    result = fvm_smedia_read(input, size, r->tag, &piece);
    if (result != FVM_S_OK)
        return result;
    if (!media_same(&r->info, &piece.info))
        return FVM_S_CONFLICT;
    offset = (size_t)(piece.part * FVM_SMEDIA_PAYLOAD);
    byte = (size_t)(piece.part / FVM_SMEDIA_BITS);
    mask = 1U << (unsigned int)(piece.part % FVM_SMEDIA_BITS);
    if (r->bitmap[byte] & mask)
    {
        if (memcmp(r->buffer + offset, piece.data, piece.size))
            return FVM_S_CONFLICT;
    }
    else
    {
        memcpy(r->buffer + offset, piece.data, piece.size);
        r->bitmap[byte] |= (unsigned char)mask;
        ++r->received;
    }
    *complete = r->received == media_parts(&r->info);
    return FVM_S_OK;
}
/* Missing-file enumeration is a policy hint, never an implicit network retransmission. */
FVM_SResult fvm_smedia_missing(const FVM_SMediaReceiver *r, unsigned long start,
                               unsigned long *parts, size_t capacity, size_t *count)
{
    unsigned long part, total;
    size_t n = 0;
    if (!media_receiver(r) || !count || (!parts && capacity))
        return FVM_S_INVALID;
    total = media_parts(&r->info);
    if (start > total)
        return FVM_S_INVALID;
    if (r->info.mode == FVM_SMEDIA_FILE)
        for (part = start; part < total && n < capacity; ++part)
            if (!(r->bitmap[part / FVM_SMEDIA_BITS] &
                  (1U << (unsigned int)(part % FVM_SMEDIA_BITS))))
                parts[n++] = part;
    *count = n;
    return FVM_S_OK;
}
