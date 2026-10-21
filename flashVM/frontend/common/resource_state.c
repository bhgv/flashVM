/* Owner-safe copied playback state, encoded independently from native widths and VM lifetimes. */
#include "resource_state.h"
#include "flashvm/srecord.h"
#include <stdlib.h>
#include <string.h>
/* Native storage owns only copied data, not contexts, roots, managed values or host callbacks. */
struct FVM_DesktopResourceState
{
    FVM_RDispatch *dispatch;              /* Borrowed queue and publication policy. */
    FVM_RHandle target;                   /* Exact registered incarnation. */
    FVM_DesktopResourceSnapshot snapshot; /* Last successfully observed boundary. */
};
/* Encode one portable nonnegative Uint32 field into adequately sized private scratch. */
static size_t state_put(unsigned char *out, unsigned long value)
{
    FVM_SInteger number;
    size_t used = 0;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}
/* Read a canonical integer without changing external output until all fields validate. */
static int state_get(const unsigned char **p, size_t *left, unsigned long *value)
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
/* Pack ten integer fields and counted source bytes; no fixed-endian/native scalar copies. */
static size_t state_encode(const FVM_DesktopResourceSnapshot *s, unsigned char *out)
{
    size_t n = 0, source = strlen(s->source);
    n += state_put(out + n, FVM_DESKTOP_STATE_VERSION);
    n += state_put(out + n, s->epoch);
    n += state_put(out + n, s->phase);
    n += state_put(out + n, s->paused);
    n += state_put(out + n, s->step_pending);
    n += state_put(out + n, s->movie_ticks);
    n += state_put(out + n, s->total_ticks);
    n += state_put(out + n, s->presentations);
    n += state_put(out + n, s->flags);
    n += state_put(out + n, (unsigned long)source);
    memcpy(out + n, s->source, source);
    return n + source;
}
/* Read-only handler never examines or retains a live player/VM object. */
static FVM_RResult state_deliver(const FVM_RRequest *request, void *out, size_t capacity,
                                 size_t *used, void *raw)
{
    FVM_DesktopResourceState *adapter = (FVM_DesktopResourceState *)raw;
    unsigned char bytes[FVM_R_BYTES];
    size_t n;
    *used = 0;
    if (request->size)
        return FVM_R_INVALID;
    if (!adapter->snapshot.epoch)
        return FVM_R_BUSY;
    n = state_encode(&adapter->snapshot, bytes);
    if (n > capacity)
        return FVM_R_LIMIT;
    memcpy(out, bytes, n);
    *used = n;
    return FVM_R_OK;
}
/* Register only after fixed native allocation succeeds; failure leaves both outputs unchanged. */
FVM_RResult fvm_desktop_resource_state_create(FVM_RDispatch *dispatch, const char *path,
                                              FVM_DesktopResourceState **out, FVM_RHandle *target)
{
    FVM_DesktopResourceState *adapter;
    FVM_RResult result;
    static const FVM_ROperation operation = {FVM_DESKTOP_STATE_SNAPSHOT, FVM_R_READ};
    if (!dispatch || !out || !target)
        return FVM_R_INVALID;
    adapter = (FVM_DesktopResourceState *)calloc(1, sizeof(*adapter));
    if (!adapter)
        return FVM_R_NOMEM;
    result = fvm_rdispatch_register(dispatch, path, &operation, 1, state_deliver, adapter,
                                    &adapter->target);
    if (result != FVM_R_OK)
    {
        free(adapter);
        return result;
    }
    adapter->dispatch = dispatch;
    *target = adapter->target;
    *out = adapter;
    return FVM_R_OK;
}
/* Saturate native-width counters explicitly so the protocol remains portable on LP64 and LLP64. */
static unsigned long state_counter(unsigned long value, unsigned long *flags)
{
    if (value > FVM_SRECORD_U32)
    {
        *flags |= FVM_DESKTOP_STATE_COUNTERS_TRUNCATED;
        return FVM_SRECORD_U32;
    }
    return value;
}
/* Observe a completed safe point; a command reply does not predict this later player state. */
FVM_RResult fvm_desktop_resource_state_observe(FVM_DesktopResourceState *adapter,
                                               const FVM_DesktopPlaybackState *state)
{
    FVM_DesktopResourceSnapshot next;
    unsigned char bytes[FVM_R_BYTES];
    size_t i, size;
    unsigned long kind = 0, sequence;
    FVM_RResult result;
    if (!adapter || !state || state->struct_size != sizeof(*state) || !state->source ||
        (state->paused != 0 && state->paused != 1) ||
        (state->step_pending != 0 && state->step_pending != 1) ||
        (state->phase != FVM_DESKTOP_PLAYBACK_READY && state->phase != FVM_DESKTOP_PLAYBACK_TURN &&
         state->phase != FVM_DESKTOP_PLAYBACK_TICK))
        return FVM_R_INVALID;
    memset(&next, 0, sizeof(next));
    next.epoch = adapter->snapshot.epoch;
    if (state->phase == FVM_DESKTOP_PLAYBACK_READY)
    {
        if (next.epoch == FVM_SRECORD_U32)
            return FVM_R_LIMIT;
        ++next.epoch;
        kind = FVM_DESKTOP_STATE_READY;
    }
    else if (!next.epoch)
        return FVM_R_BUSY;
    else if (state->phase == FVM_DESKTOP_PLAYBACK_TICK)
        kind = FVM_DESKTOP_STATE_TICK;
    else if ((unsigned long)state->paused != adapter->snapshot.paused ||
             (unsigned long)state->step_pending != adapter->snapshot.step_pending)
        kind = FVM_DESKTOP_STATE_POLICY;
    next.phase = (unsigned long)state->phase;
    next.paused = (unsigned long)state->paused;
    next.step_pending = (unsigned long)state->step_pending;
    next.movie_ticks = state_counter(state->movie_ticks, &next.flags);
    next.total_ticks = state_counter(state->total_ticks, &next.flags);
    next.presentations = state_counter(state->presentations, &next.flags);
    for (i = 0; i < FVM_DESKTOP_STATE_SOURCE && state->source[i]; ++i)
        next.source[i] = state->source[i];
    if (state->source[i])
        next.flags |= FVM_DESKTOP_STATE_SOURCE_TRUNCATED;
    if (kind)
    {
        size = state_encode(&next, bytes);
        result =
            fvm_rdispatch_publish(adapter->dispatch, adapter->target, kind, bytes, size, &sequence);
        if (result != FVM_R_OK)
            return result;
    }
    adapter->snapshot = next;
    return FVM_R_OK;
}
/* Client-side strict payload decoding; completed snapshots own all returned bytes. */
FVM_RResult fvm_desktop_resource_state_read(const void *input, size_t size,
                                            FVM_DesktopResourceSnapshot *out)
{
    const unsigned char *p = (const unsigned char *)input;
    FVM_DesktopResourceSnapshot next;
    unsigned long version, source;
    size_t left = size;
    if ((!input && size) || !out)
        return FVM_R_INVALID;
    memset(&next, 0, sizeof(next));
    if (!state_get(&p, &left, &version) || version != FVM_DESKTOP_STATE_VERSION ||
        !state_get(&p, &left, &next.epoch) || !next.epoch || !state_get(&p, &left, &next.phase) ||
        !state_get(&p, &left, &next.paused) || !state_get(&p, &left, &next.step_pending) ||
        !state_get(&p, &left, &next.movie_ticks) || !state_get(&p, &left, &next.total_ticks) ||
        !state_get(&p, &left, &next.presentations) || !state_get(&p, &left, &next.flags) ||
        !state_get(&p, &left, &source) || source > FVM_DESKTOP_STATE_SOURCE || left != source ||
        next.paused > 1 || next.step_pending > 1 || (next.flags & ~FVM_DESKTOP_STATE_FLAGS) ||
        (next.phase != FVM_DESKTOP_PLAYBACK_READY && next.phase != FVM_DESKTOP_PLAYBACK_TURN &&
         next.phase != FVM_DESKTOP_PLAYBACK_TICK) ||
        ((next.flags & FVM_DESKTOP_STATE_SOURCE_TRUNCATED) && source != FVM_DESKTOP_STATE_SOURCE) ||
        (source && memchr(p, 0, (size_t)source)))
        return FVM_R_INVALID;
    if (source)
        memcpy(next.source, p, (size_t)source);
    *out = next;
    return FVM_R_OK;
}
/* Unregister before freeing the borrowed handler userdata; queued old generations remain stale. */
FVM_RResult fvm_desktop_resource_state_destroy(FVM_DesktopResourceState **pointer)
{
    FVM_DesktopResourceState *adapter;
    FVM_RResult result;
    if (!pointer || !(adapter = *pointer))
        return FVM_R_INVALID;
    result = fvm_rdispatch_unregister(adapter->dispatch, adapter->target);
    if (result != FVM_R_OK)
        return result;
    free(adapter);
    *pointer = 0;
    return FVM_R_OK;
}
