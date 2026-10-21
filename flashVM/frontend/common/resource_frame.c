/* Copied frame resources retain old images only through bounded, session-owned native leases. */
#include "resource_frame.h"
#include "flashvm/srecord.h"
#include "flashvm/smedia.h"
#include <stdlib.h>
#include <string.h>
/* One preallocated image buffer; published data never aliases the runner's borrowed raster. */
typedef struct FrameSlot
{
    unsigned char *pixels;             /* Owned fixed-capacity packed RGBA buffer. */
    FVM_DesktopResourceFrameInfo info; /* Copied capture metadata; stored without a client lease. */
    size_t pins;                       /* Number of live leases retaining this immutable slot. */
} FrameSlot;
/* A lease belongs to one authenticated reader, not every caller knowing the numeric handle. */
typedef struct FramePin
{
    unsigned long session; /* Zero means free; session comes from the trusted dispatcher adapter. */
    unsigned long generation; /* Retired at Uint32 exhaustion rather than wrapping. */
    size_t slot;              /* Retained buffer index, meaningful only while session is nonzero. */
} FramePin;
/* Owner-only state: no VM roots, source/pixel borrows or implicit worker threads. */
struct FVM_DesktopResourceFrame
{
    FVM_RDispatch *dispatch; /* Borrowed until successful destroy. */
    FVM_RHandle target;      /* Exact resource registration incarnation. */
    size_t capacity;         /* Allocated extent of every pixel slot. */
    size_t latest;           /* Latest committed slot; valid only after sequence becomes nonzero. */
    unsigned long sequence;  /* Successful capture sequence; never wraps. */
    unsigned long dropped;   /* Saturating backpressure-drop counter. */
    FrameSlot frames[FVM_DESKTOP_RFRAME_SLOTS]; /* Fixed image storage and pin counts. */
    FramePin leases[FVM_DESKTOP_RFRAME_LEASES]; /* Bounded generation-checked reader ownership. */
};
/* Encode portable canonical unsigned metadata fields, never native-endian structures. */
static size_t frame_put(unsigned char *out, unsigned long value)
{
    FVM_SInteger number;
    size_t used = 0;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}
/* Advance a private decode cursor; callers publish output only after complete validation. */
static int frame_get(const unsigned char **p, size_t *left, unsigned long *value)
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
/* Pack seventeen integer fields followed by counted origin bytes; worst case fits R_BYTES. */
static size_t frame_encode(const FVM_DesktopResourceFrameInfo *s, unsigned char *out)
{
    size_t n = 0, length = strlen(s->source);
    n += frame_put(out + n, FVM_DESKTOP_RFRAME_VERSION);
    n += frame_put(out + n, s->lease.id);
    n += frame_put(out + n, s->lease.generation);
    n += frame_put(out + n, s->sequence);
    n += frame_put(out + n, s->format);
    n += frame_put(out + n, s->width);
    n += frame_put(out + n, s->height);
    n += frame_put(out + n, s->row_bytes);
    n += frame_put(out + n, s->byte_count);
    n += frame_put(out + n, s->movie_ticks);
    n += frame_put(out + n, s->total_ticks);
    n += frame_put(out + n, s->presentations);
    n += frame_put(out + n, s->total_presentations);
    n += frame_put(out + n, s->reason);
    n += frame_put(out + n, s->flags);
    n += frame_put(out + n, s->dropped);
    n += frame_put(out + n, (unsigned long)length);
    memcpy(out + n, s->source, length);
    return n + length;
}
/* Decode a counted request with exactly the operation-specific number of integers. */
static int frame_arguments(const FVM_RRequest *q, unsigned long *fields, size_t count)
{
    const unsigned char *p = (const unsigned char *)q->data;
    size_t left = q->size, i;
    for (i = 0; i < count; ++i)
        if (!frame_get(&p, &left, fields + i))
            return 0;
    return !left;
}
/* Resolve an exact pin incarnation before separately checking its authenticated owner. */
static FramePin *frame_pin(FVM_DesktopResourceFrame *a, unsigned long id, unsigned long generation)
{
    FramePin *pin;
    if (!id || id > FVM_DESKTOP_RFRAME_LEASES || !generation)
        return 0;
    pin = a->leases + id - 1;
    return pin->session && pin->generation == generation ? pin : 0;
}
/* Release one pin without affecting any other reader, retiring its generation on exhaustion. */
static void frame_unpin(FVM_DesktopResourceFrame *a, FramePin *pin)
{
    --a->frames[pin->slot].pins;
    pin->session = 0;
    pin->generation = pin->generation == FVM_SRECORD_U32 ? 0 : pin->generation + 1;
}
/* Execute resource reads only on the native owner; callers outside the VM see copied bytes. */
static FVM_RResult frame_deliver(const FVM_RRequest *q, void *out, size_t capacity, size_t *used,
                                 void *raw)
{
    FVM_DesktopResourceFrame *a = (FVM_DesktopResourceFrame *)raw;
    FVM_DesktopResourceFrameInfo info;
    FramePin *pin;
    unsigned long args[FVM_DESKTOP_RFRAME_REGION_FIELDS];
    unsigned char bytes[FVM_R_BYTES];
    size_t i, n, row, rows;
    *used = 0;
    if (q->operation == FVM_DESKTOP_RFRAME_INFO || q->operation == FVM_DESKTOP_RFRAME_ACQUIRE)
    {
        if (q->size)
            return FVM_R_INVALID;
        if (!a->sequence)
            return FVM_R_BUSY;
        info = a->frames[a->latest].info;
        info.dropped = a->dropped;
        i = 0;
        if (q->operation == FVM_DESKTOP_RFRAME_ACQUIRE)
        {
            for (i = 0; i < FVM_DESKTOP_RFRAME_LEASES &&
                        (a->leases[i].session || !a->leases[i].generation);
                 ++i)
            {
            }
            if (i == FVM_DESKTOP_RFRAME_LEASES)
                return FVM_R_LIMIT;
            info.lease.id = (unsigned long)i + 1;
            info.lease.generation = a->leases[i].generation;
        }
        n = frame_encode(&info, bytes);
        if (n > capacity)
            return FVM_R_LIMIT;
        if (q->operation == FVM_DESKTOP_RFRAME_ACQUIRE)
        {
            a->leases[i].session = q->session;
            a->leases[i].slot = a->latest;
            ++a->frames[a->latest].pins;
        }
        memcpy(out, bytes, n);
        *used = n;
        return FVM_R_OK;
    }
    if (q->operation != FVM_DESKTOP_RFRAME_CHUNK && q->operation != FVM_DESKTOP_RFRAME_RELEASE &&
        q->operation != FVM_DESKTOP_RFRAME_REGION)
        return FVM_R_UNKNOWN;
    if (!frame_arguments(q, args,
                         q->operation == FVM_DESKTOP_RFRAME_REGION
                             ? FVM_DESKTOP_RFRAME_REGION_FIELDS
                             : (q->operation == FVM_DESKTOP_RFRAME_CHUNK
                                    ? FVM_DESKTOP_RFRAME_CHUNK_FIELDS
                                    : FVM_DESKTOP_RFRAME_LEASE_FIELDS)))
        return FVM_R_INVALID;
    pin = frame_pin(a, args[0], args[1]);
    if (!pin)
        return FVM_R_UNKNOWN;
    if (pin->session != q->session)
        return FVM_R_DENIED;
    if (q->operation == FVM_DESKTOP_RFRAME_RELEASE)
    {
        frame_unpin(a, pin);
        return FVM_R_OK;
    }
    info = a->frames[pin->slot].info;
    if (q->operation == FVM_DESKTOP_RFRAME_REGION)
    {
        /* Subtraction bounds coordinates before any sum, product or output mutation. */
        if (!args[4] || !args[5] || args[2] >= info.width || args[3] >= info.height ||
            args[4] > info.width - args[2] || args[5] > info.height - args[3])
            return FVM_R_INVALID;
        row = (size_t)args[4] * FVM_DESKTOP_FRAME_PIXEL_BYTES;
        rows = (size_t)args[5];
        /* Reject oversized rectangles atomically; callers tile them explicitly. */
        if (row > FVM_R_BYTES || rows > FVM_R_BYTES / row || row * rows > capacity)
            return FVM_R_LIMIT;
        n = ((size_t)args[3] * (size_t)info.width + (size_t)args[2]) *
            FVM_DESKTOP_FRAME_PIXEL_BYTES;
        for (i = 0; i < rows; ++i)
            memcpy((unsigned char *)out + i * row,
                   a->frames[pin->slot].pixels + n + i * (size_t)info.row_bytes, row);
        *used = row * rows;
        return FVM_R_OK;
    }
    if (!args[3] || args[3] > FVM_R_BYTES || args[2] > info.byte_count)
        return FVM_R_INVALID;
    n = (size_t)(info.byte_count - args[2]);
    if (n > args[3])
        n = (size_t)args[3];
    if (n > capacity)
        return FVM_R_LIMIT;
    if (n)
        memcpy(out, a->frames[pin->slot].pixels + (size_t)args[2], n);
    *used = n;
    return FVM_R_OK;
}
/* Allocate every native buffer before publishing registration or either caller output. */
FVM_RResult fvm_desktop_resource_frame_create(FVM_RDispatch *dispatch, const char *path,
                                              size_t capacity, FVM_DesktopResourceFrame **out,
                                              FVM_RHandle *target)
{
    FVM_DesktopResourceFrame *a;
    FVM_RResult result;
    size_t i;
    static const FVM_ROperation operations[] = {{FVM_DESKTOP_RFRAME_INFO, FVM_R_READ},
                                                {FVM_DESKTOP_RFRAME_ACQUIRE, FVM_R_READ},
                                                {FVM_DESKTOP_RFRAME_CHUNK, FVM_R_READ},
                                                {FVM_DESKTOP_RFRAME_RELEASE, FVM_R_READ},
                                                {FVM_DESKTOP_RFRAME_REGION, FVM_R_READ}};
    if (!dispatch || !out || !target || !capacity)
        return FVM_R_INVALID;
    if (capacity > FVM_DESKTOP_RFRAME_MAX_BYTES)
        return FVM_R_LIMIT;
    a = (FVM_DesktopResourceFrame *)calloc(1, sizeof(*a));
    if (!a)
        return FVM_R_NOMEM;
    a->capacity = capacity;
    a->dispatch = dispatch;
    for (i = 0; i < FVM_DESKTOP_RFRAME_SLOTS; ++i)
    {
        a->frames[i].pixels = (unsigned char *)calloc(1, capacity);
        if (!a->frames[i].pixels)
        {
            while (i)
                free(a->frames[--i].pixels);
            free(a);
            return FVM_R_NOMEM;
        }
    }
    for (i = 0; i < FVM_DESKTOP_RFRAME_LEASES; ++i)
        a->leases[i].generation = 1;
    result = fvm_rdispatch_register(dispatch, path, operations,
                                    sizeof(operations) / sizeof(operations[0]), frame_deliver, a,
                                    &a->target);
    if (result != FVM_R_OK)
    {
        for (i = 0; i < FVM_DESKTOP_RFRAME_SLOTS; ++i)
            free(a->frames[i].pixels);
        free(a);
        return result;
    }
    *target = a->target;
    *out = a;
    return FVM_R_OK;
}
/* Saturate received native counters with an explicit wire flag rather than silently narrowing. */
static unsigned long frame_counter(unsigned long value, unsigned long *flags)
{
    if (value > FVM_SRECORD_U32)
    {
        *flags |= FVM_DESKTOP_RFRAME_COUNTERS_TRUNCATED;
        return FVM_SRECORD_U32;
    }
    return value;
}
/* Copy into a spare before publication; leased images and the previous latest remain untouched. */
FVM_RResult fvm_desktop_resource_frame_capture(FVM_DesktopResourceFrame *a,
                                               const FVM_DesktopFrame *f)
{
    FVM_DesktopResourceFrameInfo next;
    unsigned char bytes[FVM_R_BYTES];
    size_t i, row, total, n;
    unsigned long event_sequence;
    FVM_RResult result;
    if (!a || !f || f->struct_size != sizeof(*f) || !f->source || !f->presentations ||
        !f->total_presentations ||
        (f->reason != FVM_DESKTOP_FRAME_TICK && f->reason != FVM_DESKTOP_FRAME_REFRESH) ||
        f->format != FVM_DESKTOP_FRAME_RGBA8 || !f->width || !f->height ||
        f->width > FVM_DESKTOP_FRAME_MAX_DIMENSION || f->height > FVM_DESKTOP_FRAME_MAX_DIMENSION)
        return FVM_R_INVALID;
    row = (size_t)f->width * FVM_DESKTOP_FRAME_PIXEL_BYTES;
    /* Reject malformed source geometry even when reader backpressure leaves no spare slot. */
    if (!f->pixels || f->stride < row || (size_t)(f->height - 1) > ((size_t)-1 - row) / f->stride ||
        (size_t)(f->height - 1) * f->stride + row > f->byte_count)
        return FVM_R_INVALID;
    if ((size_t)f->height > ((size_t)-1) / row)
        return FVM_R_LIMIT;
    total = row * (size_t)f->height;
    if (total > a->capacity || a->sequence == FVM_SRECORD_U32)
        return FVM_R_LIMIT;
    for (i = 0; i < FVM_DESKTOP_RFRAME_SLOTS; ++i)
        if ((!a->sequence || i != a->latest) && !a->frames[i].pins)
            break;
    if (i == FVM_DESKTOP_RFRAME_SLOTS)
    {
        if (a->dropped < FVM_SRECORD_U32)
            ++a->dropped;
        return FVM_R_BUSY;
    }
    if (fvm_desktop_frame_copy(f, a->frames[i].pixels, a->capacity, row) != FVM_OK)
        return FVM_R_INVALID;
    memset(&next, 0, sizeof(next));
    next.sequence = a->sequence + 1;
    next.format = f->format;
    next.width = f->width;
    next.height = f->height;
    next.row_bytes = (unsigned long)row;
    next.byte_count = (unsigned long)total;
    next.reason = f->reason;
    next.dropped = a->dropped;
    next.movie_ticks = frame_counter(f->movie_ticks, &next.flags);
    next.total_ticks = frame_counter(f->total_ticks, &next.flags);
    next.presentations = frame_counter(f->presentations, &next.flags);
    next.total_presentations = frame_counter(f->total_presentations, &next.flags);
    for (n = 0; n < FVM_DESKTOP_RFRAME_SOURCE && f->source[n]; ++n)
        next.source[n] = f->source[n];
    if (f->source[n])
        next.flags |= FVM_DESKTOP_RFRAME_SOURCE_TRUNCATED;
    n = frame_encode(&next, bytes);
    result = fvm_rdispatch_publish(a->dispatch, a->target, FVM_DESKTOP_RFRAME_PRESENTED, bytes, n,
                                   &event_sequence);
    if (result != FVM_R_OK)
        return result;
    a->frames[i].info = next;
    a->latest = i;
    a->sequence = next.sequence;
    return FVM_R_OK;
}
/* A slow reader may drop export without blocking or failing the player's own raster pipeline. */
FVM_Result fvm_desktop_resource_frame_notify(const FVM_DesktopFrame *frame, void *raw)
{
    FVM_RResult result = fvm_desktop_resource_frame_capture((FVM_DesktopResourceFrame *)raw, frame);
    if (result == FVM_R_OK || result == FVM_R_BUSY)
        return FVM_OK;
    return result == FVM_R_INVALID ? FVM_INVALID_ARGUMENT : FVM_ERROR;
}
/* Strict native-independent metadata decoder; no pointer from input escapes on success. */
FVM_RResult fvm_desktop_resource_frame_read(const void *input, size_t size,
                                            FVM_DesktopResourceFrameInfo *out)
{
    const unsigned char *p = (const unsigned char *)input;
    size_t left = size;
    FVM_DesktopResourceFrameInfo next;
    unsigned long version, source;
    if ((!input && size) || !out)
        return FVM_R_INVALID;
    memset(&next, 0, sizeof(next));
    if (!frame_get(&p, &left, &version) || version != FVM_DESKTOP_RFRAME_VERSION ||
        !frame_get(&p, &left, &next.lease.id) || !frame_get(&p, &left, &next.lease.generation) ||
        !frame_get(&p, &left, &next.sequence) || !frame_get(&p, &left, &next.format) ||
        !frame_get(&p, &left, &next.width) || !frame_get(&p, &left, &next.height) ||
        !frame_get(&p, &left, &next.row_bytes) || !frame_get(&p, &left, &next.byte_count) ||
        !frame_get(&p, &left, &next.movie_ticks) || !frame_get(&p, &left, &next.total_ticks) ||
        !frame_get(&p, &left, &next.presentations) ||
        !frame_get(&p, &left, &next.total_presentations) || !frame_get(&p, &left, &next.reason) ||
        !frame_get(&p, &left, &next.flags) || !frame_get(&p, &left, &next.dropped) ||
        !frame_get(&p, &left, &source) || !next.sequence ||
        next.format != FVM_DESKTOP_FRAME_RGBA8 || !next.width || !next.height ||
        next.width > FVM_DESKTOP_FRAME_MAX_DIMENSION ||
        next.height > FVM_DESKTOP_FRAME_MAX_DIMENSION ||
        next.row_bytes != next.width * FVM_DESKTOP_FRAME_PIXEL_BYTES ||
        next.byte_count != next.row_bytes * next.height ||
        next.lease.id > FVM_DESKTOP_RFRAME_LEASES || (!next.lease.id) != (!next.lease.generation) ||
        !next.presentations || !next.total_presentations ||
        (next.reason != FVM_DESKTOP_FRAME_TICK && next.reason != FVM_DESKTOP_FRAME_REFRESH) ||
        (next.flags & ~FVM_DESKTOP_RFRAME_FLAGS) || source > FVM_DESKTOP_RFRAME_SOURCE ||
        source != left ||
        ((next.flags & FVM_DESKTOP_RFRAME_SOURCE_TRUNCATED) &&
         source != FVM_DESKTOP_RFRAME_SOURCE) ||
        (source && memchr(p, 0, (size_t)source)))
        return FVM_R_INVALID;
    if (source)
        memcpy(next.source, p, (size_t)source);
    *out = next;
    return FVM_R_OK;
}
/* Revoke current authority and retire every pin owned by that authenticated connection. */
FVM_RResult fvm_desktop_resource_frame_revoke(FVM_DesktopResourceFrame *a, unsigned long session)
{
    FVM_RResult result;
    size_t i;
    if (!a)
        return FVM_R_INVALID;
    result = fvm_rdispatch_grant(a->dispatch, a->target, session, 0);
    if (result != FVM_R_OK)
        return result;
    for (i = 0; i < FVM_DESKTOP_RFRAME_LEASES; ++i)
        if (a->leases[i].session == session)
            frame_unpin(a, a->leases + i);
    return FVM_R_OK;
}
/* Native pins are invalidated with resource identity before any copied pixel storage is freed. */
FVM_RResult fvm_desktop_resource_frame_destroy(FVM_DesktopResourceFrame **pointer)
{
    FVM_DesktopResourceFrame *a;
    FVM_RResult result;
    size_t i;
    if (!pointer || !(a = *pointer))
        return FVM_R_INVALID;
    result = fvm_rdispatch_unregister(a->dispatch, a->target);
    if (result != FVM_R_OK)
        return result;
    for (i = 0; i < FVM_DESKTOP_RFRAME_SLOTS; ++i)
        free(a->frames[i].pixels);
    free(a);
    *pointer = 0;
    return FVM_R_OK;
}

/* Export exact retained frame bytes only after current resource rights and agreement validation. */
FVM_RResult fvm_desktop_resource_frame_write_part(
    FVM_DesktopResourceFrame *a, unsigned long session, FVM_DesktopFrameLease lease,
    const FVM_SControlExchange *agreement, unsigned long mode, unsigned long time_ms,
    unsigned long part, void *output, size_t capacity, size_t *used)
{
    FramePin *pin;
    FrameSlot *frame;
    FVM_SControlLimits limits;
    FVM_SControlBoundary boundary;
    FVM_SMediaProfile profile;
    FVM_SMediaInfo info;
    FVM_RResult rights;
    FVM_SResult result;
    size_t operation;
    unsigned long count, offset, bytes;
    if (!a || !output || !used)
        return FVM_R_INVALID;
    rights = fvm_rdispatch_operation_index(a->dispatch, session, a->target,
                                           FVM_DESKTOP_RFRAME_CHUNK, &operation);
    if (rights != FVM_R_OK)
        return rights;
    pin = frame_pin(a, lease.id, lease.generation);
    if (!pin)
        return FVM_R_UNKNOWN;
    if (pin->session != session)
        return FVM_R_DENIED;
    result = fvm_scontrol_active(agreement, &limits, &boundary);
    if (result != FVM_S_OK)
        return result == FVM_S_UNKNOWN ? FVM_R_BUSY : FVM_R_INVALID;
    if (!limits.slot_bytes || !boundary.stream)
        return FVM_R_INVALID;
    frame = a->frames + pin->slot;
    if (frame->info.sequence < boundary.object)
        return FVM_R_CONFLICT;
    if (frame->info.byte_count > FVM_SMEDIA_MAX_BYTES)
        return FVM_R_LIMIT;
    count = (frame->info.byte_count - 1) / limits.part_bytes + 1;
    if (count > FVM_SMEDIA_PARTS)
        return FVM_R_LIMIT;
    if (part >= count)
        return FVM_R_INVALID;
    offset = part * limits.part_bytes;
    bytes = frame->info.byte_count - offset;
    if (bytes > limits.part_bytes)
        bytes = limits.part_bytes;
    profile.slot_bytes = (size_t)limits.slot_bytes;
    profile.part_bytes = limits.part_bytes;
    info.mode = mode;
    info.stream = boundary.stream;
    info.epoch = boundary.epoch;
    info.object = frame->info.sequence;
    info.time_ms = time_ms;
    info.total = frame->info.byte_count;
    result =
        fvm_smedia_write_profile(&profile, limits.tag, &info, part, frame->pixels + (size_t)offset,
                                 (size_t)bytes, output, capacity, used);
    return result == FVM_S_OK ? FVM_R_OK : (result == FVM_S_LIMIT ? FVM_R_LIMIT : FVM_R_INVALID);
}
