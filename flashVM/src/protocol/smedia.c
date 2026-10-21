/* Original bounded object reassembly over fixed checked slots; transport and codecs stay separate.
 */
#include "flashvm/smedia.h"
#include <string.h>
/* Default geometry is opt-in and remains stable for existing convenience calls. */
static const FVM_SMediaProfile media_default = {FVM_SSLOT_BYTES, FVM_SMEDIA_PAYLOAD};
/* Profile comes from authenticated control agreement, never from a damaged part header. */
static int media_profile(const FVM_SMediaProfile *p)
{
    return p && p->slot_bytes >= FVM_SSLOT_MIN_BYTES && p->slot_bytes <= FVM_SSLOT_MAX_BYTES &&
           p->part_bytes && p->part_bytes <= p->slot_bytes - FVM_SMEDIA_RESERVE;
}
/* Validate portable metadata before arithmetic, buffer access or session adoption. */
static int media_info(const FVM_SMediaInfo *info)
{
    return info && (info->mode == FVM_SMEDIA_FILE || info->mode == FVM_SMEDIA_LIVE) &&
           info->stream && info->stream <= FVM_SRECORD_U32 && info->epoch &&
           info->epoch <= FVM_SRECORD_U32 && info->object && info->object <= FVM_SRECORD_U32 &&
           info->time_ms <= FVM_SRECORD_U32 && info->total && info->total <= FVM_SMEDIA_MAX_BYTES;
}
/* Ceiling division is safe after the strict object-size validation. */
static unsigned long media_parts(const FVM_SMediaInfo *info, const FVM_SMediaProfile *p)
{
    return (info->total - 1) / p->part_bytes + 1;
}
/* A final fragment has an exact short extent; no overlapping or sparse peer-selected offsets. */
static size_t media_size(const FVM_SMediaInfo *info, unsigned long part, const FVM_SMediaProfile *p)
{
    unsigned long remaining = info->total - part * p->part_bytes;
    return (size_t)(remaining < p->part_bytes ? remaining : p->part_bytes);
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
FVM_SResult fvm_smedia_write_profile(const FVM_SMediaProfile *profile, unsigned long tag,
                                     const FVM_SMediaInfo *info, unsigned long part,
                                     const void *data, size_t size, void *output, size_t capacity,
                                     size_t *used)
{
    unsigned char payload[FVM_SMEDIA_FIELDS * FVM_SRECORD_INTEGER_MAX];
    size_t n = 0, encoded;
    FVM_SResult result;
    if (!media_profile(profile) || !output || !used || !tag || tag > FVM_SRECORD_U32 ||
        !media_info(info) || !data || media_parts(info, profile) > FVM_SMEDIA_PARTS ||
        part >= media_parts(info, profile) || size != media_size(info, part, profile))
        return FVM_S_INVALID;
    if (capacity < profile->slot_bytes)
        return FVM_S_LIMIT;
    n += media_put(payload + n, FVM_SMEDIA_VERSION);
    n += media_put(payload + n, info->mode);
    n += media_put(payload + n, info->stream);
    n += media_put(payload + n, info->epoch);
    n += media_put(payload + n, info->object);
    n += media_put(payload + n, info->time_ms);
    n += media_put(payload + n, info->total);
    n += media_put(payload + n, part);
    result = fvm_srecord_write_pair(tag, payload, n, data, size, (unsigned char *)output,
                                    profile->slot_bytes, &encoded);
    if (result != FVM_S_OK)
        return result;
    memset((unsigned char *)output + encoded, 0, profile->slot_bytes - encoded);
    *used = profile->slot_bytes;
    return FVM_S_OK;
}
/* Decode without accepting tag declarations from a lossy/untrusted media datagram. */
FVM_SResult fvm_smedia_read_profile(const FVM_SMediaProfile *profile, const void *input,
                                    size_t size, unsigned long tag, FVM_SMediaPiece *out)
{
    FVM_SRecord record;
    FVM_SMediaPiece piece;
    FVM_SResult result;
    unsigned long version;
    const unsigned char *p;
    size_t used, left;
    if (!media_profile(profile) || !out || !tag || tag > FVM_SRECORD_U32)
        return FVM_S_INVALID;
    result = fvm_sslot_read_sized(input, size, profile->slot_bytes, &record, &used);
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
        media_parts(&piece.info, profile) > FVM_SMEDIA_PARTS ||
        piece.part >= media_parts(&piece.info, profile) ||
        left != media_size(&piece.info, piece.part, profile))
        return FVM_S_INVALID;
    piece.data = p;
    piece.size = left;
    *out = piece;
    return FVM_S_OK;
}
/* Validate the receiver's native storage contract; callers must not modify private state. */
static int media_receiver(const FVM_SMediaReceiver *r)
{
    return r && r->struct_size == sizeof(*r) && media_info(&r->info) &&
           media_profile(&r->profile) && r->tag && r->tag <= FVM_SRECORD_U32 && r->buffer &&
           r->capacity >= r->info.total && media_parts(&r->info, &r->profile) <= FVM_SMEDIA_PARTS &&
           r->received <= media_parts(&r->info, &r->profile);
}
/* Replace identity/bitmap only after all caller-owned buffer and capacity checks pass. */
FVM_SResult fvm_smedia_receiver_init_profile(FVM_SMediaReceiver *out,
                                             const FVM_SMediaProfile *profile, unsigned long tag,
                                             const FVM_SMediaInfo *info, void *buffer,
                                             size_t capacity)
{
    FVM_SMediaReceiver r;
    if (!out || !media_profile(profile) || !tag || tag > FVM_SRECORD_U32 || !media_info(info) ||
        media_parts(info, profile) > FVM_SMEDIA_PARTS || !buffer)
        return FVM_S_INVALID;
    if (capacity < info->total)
        return FVM_S_LIMIT;
    memset(&r, 0, sizeof(r));
    r.struct_size = sizeof(r);
    r.info = *info;
    r.profile = *profile;
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
    result = fvm_smedia_read_profile(&r->profile, input, size, r->tag, &piece);
    if (result != FVM_S_OK)
        return result;
    if (!media_same(&r->info, &piece.info))
        return FVM_S_CONFLICT;
    offset = (size_t)(piece.part * r->profile.part_bytes);
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
    *complete = r->received == media_parts(&r->info, &r->profile);
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
    total = media_parts(&r->info, &r->profile);
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

/* Default profile wrappers retain the original 1024/768 bulk format. */
FVM_SResult fvm_smedia_write(unsigned long tag, const FVM_SMediaInfo *info, unsigned long part,
                             const void *data, size_t size, void *out, size_t capacity,
                             size_t *used)
{
    return fvm_smedia_write_profile(&media_default, tag, info, part, data, size, out, capacity,
                                    used);
}
/* Default profile view still borrows bytes, with no implicit negotiation. */
FVM_SResult fvm_smedia_read(const void *input, size_t size, unsigned long tag, FVM_SMediaPiece *out)
{
    return fvm_smedia_read_profile(&media_default, input, size, tag, out);
}
/* Default receiver copies the built-in profile exactly as an explicitly agreed profile would. */
FVM_SResult fvm_smedia_receiver_init(FVM_SMediaReceiver *out, unsigned long tag,
                                     const FVM_SMediaInfo *info, void *buffer, size_t capacity)
{
    return fvm_smedia_receiver_init_profile(out, &media_default, tag, info, buffer, capacity);
}

/* Committed metadata selects the geometry; a late old packet can never select it itself. */
FVM_SResult fvm_smedia_receiver_init_agreed(FVM_SMediaReceiver *out,
                                            const FVM_SControlExchange *agreement,
                                            const FVM_SMediaInfo *info, void *buffer,
                                            size_t capacity)
{
    FVM_SMediaProfile profile;
    const FVM_SControlBoundary *boundary;
    if (!agreement || agreement->struct_size != sizeof(*agreement) || !agreement->active_id ||
        !agreement->active.slot_bytes || agreement->active.slot_bytes > (size_t)-1 ||
        !media_info(info))
        return FVM_S_INVALID;
    boundary = &agreement->active_boundary;
    if (boundary->stream && (boundary->stream != info->stream || boundary->epoch != info->epoch ||
                             info->object < boundary->object))
        return FVM_S_CONFLICT;
    profile.slot_bytes = (size_t)agreement->active.slot_bytes;
    profile.part_bytes = agreement->active.part_bytes;
    return fvm_smedia_receiver_init_profile(out, &profile, agreement->active.tag, info, buffer,
                                            capacity);
}
