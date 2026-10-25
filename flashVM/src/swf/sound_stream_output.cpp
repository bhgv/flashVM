/* Bounded stream producer/retry ownership and explicit native FIFO dequeue-clock accounting. */
#include "flashvm/swf_stream_output.h"
#include "sound_mix_queue_internal.h"
#include <cfloat>
#include <cstring>


/* Native fields are host-private after successful binding. */
static bool stream_output(const FVM_SWFStreamOutput *s)
{
    return s && s->struct_size == sizeof(*s) && s->converter && s->pcm && !s->busy;
}


/* Restore callback guard even when a C++ backend throws through a C function pointer. */
struct StreamBusy
{
    FVM_SWFStreamOutput *state; /* Borrow the serial native owner for one public operation. */


    explicit StreamBusy(FVM_SWFStreamOutput *s) : state(s)
    {
        state->busy = 1;
    }


    ~StreamBusy()
    {
        state->busy = 0;
    }
};


/* Validate monotone consumption before publishing any clock observation. */
static FVM_Result stream_observe(FVM_SWFStreamOutput *s)
{
    size_t queued = 0;
    FVM_Result result = s->backend.queued(s->backend.userdata, &queued);
    if (result != FVM_OK)
    {
        return result;
    }
    if (queued > s->target || (double)queued > s->info.submitted_frames)
    {
        return FVM_ERROR;
    }
    double played = s->info.submitted_frames - (double)queued;
    if (played < s->info.played_frames)
    {
        return FVM_ERROR;
    }
    s->info.queued_frames = queued;
    s->info.played_frames = played;
    s->info.seconds = played / s->backend.sample_rate;
    return FVM_OK;
}


/* [GC:] Bind exclusively borrowed native objects; no VM root, managed pointer or allocation. */
extern "C" FVM_Result fvm_swf_stream_output_init(FVM_SWFStreamOutput *out,
                                                 FVM_SWFStreamConverter *converter,
                                                 const FVM_AudioQueueBackend *backend,
                                                 short *pcm,
                                                 size_t capacity,
                                                 size_t block,
                                                 size_t target)
{
    if (!out || !converter || converter->struct_size != sizeof(*converter) || !backend ||
        backend->struct_size != sizeof(*backend) || backend->abi_version != FVM_AUDIO_QUEUE_ABI_VERSION ||
        !backend->write || !backend->queued || !backend->pause || !backend->clear || !pcm ||
        backend->sample_rate != converter->output_rate || backend->channels != converter->output_channels || !block ||
        block > FVM_STREAM_CONVERT_MAX_FRAMES || target < block || target > FVM_SWF_SOUND_OUTPUT_MAX_QUEUE ||
        !backend->channels || block > capacity / backend->channels)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (DBL_MANT_DIG < FVM_SWF_SOUND_OUTPUT_CLOCK_BITS)
    {
        return FVM_UNSUPPORTED;
    }
    /* A copied position validates the whole converter descriptor without changing its cursor. */
    FVM_SWFStreamConverter checked = *converter;
    FVM_Result result = fvm_swf_stream_converter_seek(&checked, checked.block_count, 0);
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_SWFStreamOutput next;
    std::memset(&next, 0, sizeof(next));
    next.struct_size = sizeof(next);
    next.converter = converter;
    next.backend = *backend;
    next.pcm = pcm;
    next.capacity = capacity;
    next.block = block;
    next.target = target;
    next.info.struct_size = sizeof(next.info);
    next.info.paused = 1;
    next.info.chunk.struct_size = sizeof(next.info.chunk);
    try
    {
        size_t queued = 0;
        result = backend->queued(backend->userdata, &queued);
        if (result != FVM_OK)
        {
            return result;
        }
        if (queued)
        {
            return FVM_INVALID_ARGUMENT;
        }
        result = backend->pause(backend->userdata, 1);
        if (result != FVM_OK)
        {
            return result;
        }
        *out = next;
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}


/* Read trusted prepared metadata; pending bytes remain the next operation until accepted/reset. */
static FVM_Result stream_next_frame(const FVM_SWFStreamOutput *s, unsigned int *out)
{
    if (s->info.pending_frames)
    {
        *out = s->info.chunk.frame;
        return FVM_OK;
    }
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    FVM_Result result =
        fvm_swf_stream_sound_snapshot_block_at(s->converter->snapshot, s->converter->block_index, &block);
    if (result == FVM_OK)
    {
        *out = block.frame;
    }
    return result;
}


/* Querying metadata neither consumes source PCM nor calls an external backend. */
extern "C" FVM_Result fvm_swf_stream_output_next_frame(const FVM_SWFStreamOutput *s, unsigned int *out)
{
    if (!stream_output(s) || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return stream_next_frame(s, out);
}


/* Keep a rejected prepared block stable until a later all-or-none backend acceptance. */
static FVM_Result stream_pump_locked(FVM_SWFStreamOutput *s, const unsigned int *limit, size_t *accepted)
{
    try
    {
        FVM_Result result = stream_observe(s);
        if (result != FVM_OK)
        {
            return result;
        }
        if (s->info.eof)
        {
            return FVM_NOT_FOUND;
        }
        if (limit)
        {
            unsigned int next;
            result = stream_next_frame(s, &next);
            if (result == FVM_NOT_FOUND)
            {
                s->info.eof = 1;
                return result;
            }
            if (result != FVM_OK)
            {
                return result;
            }
            if (next > *limit)
            {
                return FVM_NOT_FOUND;
            }
        }
        size_t available = s->target - s->info.queued_frames;
        if (!available || s->info.pending_frames > available)
        {
            return FVM_NOT_FOUND;
        }
        if (!s->info.pending_frames)
        {
            size_t count = available < s->block ? available : s->block;
            if (s->info.submitted_frames > FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - (double)count)
            {
                return FVM_ERROR;
            }
            FVM_SWFStreamConvertedChunk chunk;
            chunk.struct_size = sizeof(chunk);
            size_t written;
            result = fvm_swf_stream_converter_next(s->converter, count, s->pcm, s->capacity, &chunk, &written);
            if (result == FVM_NOT_FOUND)
            {
                s->info.eof = 1;
                return result;
            }
            if (result != FVM_OK)
            {
                return result;
            }
            s->info.chunk = chunk;
            s->info.pending_frames = chunk.frames;
            if (!chunk.frames)
            {
                return FVM_OK;
            }
        }
        size_t count = s->info.pending_frames;
        result = s->backend.write(s->backend.userdata, s->pcm, count);
        if (result != FVM_OK)
        {
            return result;
        }
        s->info.submitted_frames += count;
        s->info.queued_frames += count;
        s->info.pending_frames = 0;
        if (accepted)
        {
            *accepted = count;
        }
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}


/* Acquire exactly one stream callback barrier for the original public producer operations. */
static FVM_Result stream_pump(FVM_SWFStreamOutput *s, const unsigned int *limit, size_t *accepted)
{
    if (accepted)
    {
        *accepted = 0;
    }
    if (!stream_output(s))
    {
        return FVM_INVALID_ARGUMENT;
    }


    StreamBusy guard(s);
    return stream_pump_locked(s, limit, accepted);
}


/* Preserve the original unrestricted native FIFO producer entry point. */
extern "C" FVM_Result fvm_swf_stream_output_pump(FVM_SWFStreamOutput *s, size_t *accepted)
{
    return stream_pump(s, 0, accepted);
}


/* Authorize only actual source movie frames through an explicit host-selected boundary. */
extern "C" FVM_Result fvm_swf_stream_output_pump_until(FVM_SWFStreamOutput *s, unsigned int frame, size_t *accepted)
{
    return stream_pump(s, &frame, accepted);
}


/* Validate that neither producer has independently advanced the exclusive composition epoch. */
static FVM_Result stream_mix_clock(FVM_SWFStreamOutput *s, FVM_SWFSoundMixQueue *mix, FVM_SWFSoundMixQueueClock &clock)
{
    clock.struct_size = sizeof(clock);
    FVM_Result result = fvm_swf_sound_mix_queue_clock_info(mix, &clock);
    if (result != FVM_OK)
    {
        return result;
    }
    if (clock.submitted_frames != s->info.submitted_frames || clock.played_frames < s->info.played_frames ||
        clock.queued_frames > s->target)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return FVM_OK;
}


/* Caller holds StreamBusy; account exactly one acknowledged zero-base event block. */
static FVM_Result
stream_mix_tail_locked(FVM_SWFStreamOutput *s, FVM_SWFSoundMixQueue *mix, size_t *accepted, bool finite = false)
{
    FVM_SWFSoundMixQueueClock clock;
    FVM_Result result = stream_mix_clock(s, mix, clock);
    if (result != FVM_OK)
    {
        return result;
    }
    size_t count = 0;
    result = fvm_swf_sound_mix_queue_pump_tail_observed(mix, s->block, s->target, &count, &clock, finite);
    if (result != FVM_OK)
    {
        return result;
    }
    s->info.submitted_frames += (double)count;
    s->info.played_frames = clock.played_frames;
    s->info.seconds = clock.seconds;
    s->info.queued_frames = clock.queued_frames;
    if (accepted)
    {
        *accepted = count;
    }
    return FVM_OK;
}


/* Preserve the explicit final-EOF entry point and its original source validation. */
extern "C" FVM_Result
fvm_swf_stream_output_pump_mix_tail(FVM_SWFStreamOutput *s, FVM_SWFSoundMixQueue *mix, size_t *accepted)
{
    if (accepted)
    {
        *accepted = 0;
    }
    if (!stream_output(s) || !s->info.eof || s->info.pending_frames ||
        !fvm_swf_sound_mix_queue_matches(mix, &s->backend))
    {
        return FVM_INVALID_ARGUMENT;
    }


    StreamBusy guard(s);
    return stream_mix_tail_locked(s, mix, accepted);
}


/* Choose authorized base or gated events without allowing a tail retry to become base input. */
static FVM_Result stream_pump_mix_until(
    FVM_SWFStreamOutput *s, FVM_SWFSoundMixQueue *mix, unsigned int frame, size_t *accepted, bool finite)
{
    if (accepted)
    {
        *accepted = 0;
    }
    if (!stream_output(s) || !fvm_swf_sound_mix_queue_matches(mix, &s->backend))
    {
        return FVM_INVALID_ARGUMENT;
    }


    StreamBusy guard(s);
    FVM_SWFSoundMixQueueClock clock;
    FVM_Result result = stream_mix_clock(s, mix, clock);
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_SWFSoundMixQueuePending pending;
    pending.struct_size = sizeof(pending);
    result = fvm_swf_sound_mix_queue_pending(mix, &pending);
    if (result != FVM_OK)
    {
        return result;
    }
    if (pending.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL)
    {
        if (s->info.pending_frames)
        {
            return FVM_INVALID_ARGUMENT;
        }
        return stream_mix_tail_locked(s, mix, accepted, finite);
    }
    if (pending.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE && pending.frames != s->info.pending_frames)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = stream_pump_locked(s, &frame, accepted);
    if (result == FVM_OK)
    { /* Origin-enabled writes may observe more dequeue during their own admission check. */
        fvm_swf_sound_mix_queue_last_clock(mix, &clock);
        s->info.played_frames = clock.played_frames;
        s->info.seconds = clock.seconds;
        s->info.queued_frames = clock.queued_frames;
    }
    if (result != FVM_NOT_FOUND || s->info.pending_frames)
    {
        return result;
    }
    unsigned int next = 0;
    result = stream_next_frame(s, &next);
    if (result != FVM_NOT_FOUND && result != FVM_OK)
    {
        return result;
    }
    if (result == FVM_OK && next <= frame)
    {
        return FVM_NOT_FOUND;
    }
    /* Only a true frame hold or source EOF authorizes zero-base events, never base backpressure. */
    return stream_mix_tail_locked(s, mix, accepted, finite);
}


/* Keep the existing adaptive/padded scheduling ABI and source gate policy. */
extern "C" FVM_Result fvm_swf_stream_output_pump_mix_until(FVM_SWFStreamOutput *s,
                                                           FVM_SWFSoundMixQueue *mix,
                                                           unsigned int frame,
                                                           size_t *accepted)
{
    return stream_pump_mix_until(s, mix, frame, accepted, false);
}


/* Opt in to a finite voice horizon without changing base authorization or retained retry order. */
extern "C" FVM_Result fvm_swf_stream_output_pump_mix_finite_until(FVM_SWFStreamOutput *s,
                                                                  FVM_SWFSoundMixQueue *mix,
                                                                  unsigned int frame,
                                                                  size_t *accepted)
{
    return stream_pump_mix_until(s, mix, frame, accepted, true);
}


/* Pause is explicit: priming/resume policy belongs to the embedding host. */
extern "C" FVM_Result fvm_swf_stream_output_pause(FVM_SWFStreamOutput *s, int paused)
{
    if (!stream_output(s) || (paused != 0 && paused != 1))
    {
        return FVM_INVALID_ARGUMENT;
    }


    StreamBusy guard(s);
    try
    {
        FVM_Result result = s->backend.pause(s->backend.userdata, paused);
        if (result == FVM_OK)
        {
            s->info.paused = paused;
        }
        return result;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}


/* An observed EOF is separate from draining the last accepted FIFO bytes. */
extern "C" FVM_Result fvm_swf_stream_output_info(FVM_SWFStreamOutput *s, FVM_SWFStreamOutputInfo *out)
{
    if (!stream_output(s) || !out || out->struct_size != sizeof(*out))
    {
        return FVM_INVALID_ARGUMENT;
    }


    StreamBusy guard(s);
    try
    {
        FVM_Result result = stream_observe(s);
        if (result == FVM_OK)
        {
            *out = s->info;
        }
        return result;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}


/* [GC:] Copy one paired native observation without rendering or touching managed objects. */
extern "C" FVM_Result
fvm_swf_stream_output_mix_info(FVM_SWFStreamOutput *s, FVM_SWFSoundMixQueue *mix, FVM_SWFStreamMixInfo *out)
{
    if (!stream_output(s) || !out || out->struct_size != sizeof(*out) ||
        !fvm_swf_sound_mix_queue_matches(mix, &s->backend))
    {
        return FVM_INVALID_ARGUMENT;
    }


    StreamBusy guard(s);
    FVM_SWFSoundMixQueueClock clock;
    fvm_swf_sound_mix_queue_last_clock(mix, &clock);
    if (clock.submitted_frames != s->info.submitted_frames || clock.played_frames < s->info.played_frames)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_SWFStreamMixInfo info;
    std::memset(&info, 0, sizeof(info));
    info.struct_size = sizeof(info);
    info.origins.struct_size = sizeof(info.origins);
    FVM_Result result = fvm_swf_sound_mix_queue_origin_info(mix, &info.origins);
    if (result != FVM_OK)
    {
        return result;
    }
    fvm_swf_sound_mix_queue_last_clock(mix, &clock);
    if (clock.queued_frames > s->target)
    {
        return FVM_INVALID_ARGUMENT;
    }
    info.stream = s->info;
    info.stream.played_frames = clock.played_frames;
    info.stream.seconds = clock.seconds;
    info.stream.queued_frames = clock.queued_frames;
    s->info = info.stream;
    *out = info;
    return FVM_OK;
}


/* Check a prospective cursor on a copy before any native queue or binding change. */
static FVM_Result stream_match(FVM_SWFStreamOutput *s, FVM_SWFStreamConverter *converter)
{
    if (!converter || converter->struct_size != sizeof(*converter) ||
        converter->output_rate != s->backend.sample_rate || converter->output_channels != s->backend.channels)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_SWFStreamConverter checked = *converter;
    return fvm_swf_stream_converter_seek(&checked, checked.block_count, 0);
}


/* Clear/seek is staged: successful pause remains visible when clear fails. */
static FVM_Result stream_seek(FVM_SWFStreamOutput *s, FVM_SWFStreamConverter *target, size_t block, size_t first)
{
    FVM_Result result = stream_match(s, target);
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_SWFStreamConverter next = *target;
    result = fvm_swf_stream_converter_seek(&next, block, first);
    if (result != FVM_OK)
    {
        return result;
    }


    StreamBusy guard(s);
    try
    {
        result = s->backend.pause(s->backend.userdata, 1);
        if (result != FVM_OK)
        {
            return result;
        }
        s->info.paused = 1;
        result = s->backend.clear(s->backend.userdata);
        if (result != FVM_OK)
        {
            return result;
        }
        *target = next;
        s->converter = target;
        std::memset(&s->info, 0, sizeof(s->info));
        s->info.struct_size = sizeof(s->info);
        s->info.paused = 1;
        s->info.chunk.struct_size = sizeof(s->info.chunk);
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}


/* Resolve trusted block metadata without backend effects or target cursor publication. */
static FVM_Result stream_frame_block(FVM_SWFStreamConverter *converter, unsigned int frame, size_t &first)
{
    first = 0;
    size_t last = converter->block_count;
    while (first < last)
    {
        size_t middle = first + (last - first) / 2;
        FVM_SWFStreamSoundBlock block;
        block.struct_size = sizeof(block);
        FVM_Result result = fvm_swf_stream_sound_snapshot_block_at(converter->snapshot, middle, &block);
        if (result != FVM_OK)
        {
            return result;
        }
        if (block.frame < frame)
        {
            first = middle + 1;
        }
        else
        {
            last = middle;
        }
    }
    return FVM_OK;
}


/* Preserve the existing absolute current-source seek entry point. */
extern "C" FVM_Result fvm_swf_stream_output_seek(FVM_SWFStreamOutput *s, size_t block, size_t first)
{
    if (!stream_output(s))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return stream_seek(s, s->converter, block, first);
}


/* Resolve nonsequential navigation without exposing source block ordinals to a native host. */
extern "C" FVM_Result fvm_swf_stream_output_seek_frame(FVM_SWFStreamOutput *s, unsigned int frame)
{
    if (!stream_output(s))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_swf_stream_output_bind_frame(s, s->converter, frame);
}


/* Switch exhausted sources without clearing accepted PCM or restarting queue accounting. */
extern "C" FVM_Result fvm_swf_stream_output_continue(FVM_SWFStreamOutput *s, FVM_SWFStreamConverter *next)
{
    if (!stream_output(s) || next == s->converter || s->info.pending_frames)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Result result = stream_match(s, next);
    if (result != FVM_OK)
    {
        return result;
    }
    unsigned int frame;
    result = stream_next_frame(s, &frame);
    if (result != FVM_NOT_FOUND)
    {
        return result == FVM_OK ? FVM_INVALID_ARGUMENT : result;
    }
    s->converter = next;
    s->info.eof = 0;
    return FVM_OK;
}


/* Validate first, then publish a different source/frame only after acknowledged native clear. */
extern "C" FVM_Result
fvm_swf_stream_output_bind_frame(FVM_SWFStreamOutput *s, FVM_SWFStreamConverter *target, unsigned int frame)
{
    if (!stream_output(s) || !frame)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Result result = stream_match(s, target);
    if (result != FVM_OK)
    {
        return result;
    }
    size_t block;
    result = stream_frame_block(target, frame, block);
    if (result != FVM_OK)
    {
        return result;
    }
    return stream_seek(s, target, block, 0);
}


/* Validate signed authored position before any backend effects; reuse staged publication. */
extern "C" FVM_Result
fvm_swf_stream_output_bind_frame_aligned(FVM_SWFStreamOutput *s, FVM_SWFStreamConverter *target, unsigned int frame)
{
    if (!stream_output(s) || !frame)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Result result = stream_match(s, target);
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_SWFStreamSoundSeekPoint point;
    point.struct_size = sizeof(point);
    result = fvm_swf_stream_sound_snapshot_seek_point(target->snapshot, frame, &point);
    if (result != FVM_OK)
    {
        return result;
    }
    return stream_seek(s, target, point.block_index, point.first);
}


/* Preserve native ownership and busy checks for current-source aligned navigation. */
extern "C" FVM_Result fvm_swf_stream_output_seek_frame_aligned(FVM_SWFStreamOutput *s, unsigned int frame)
{
    if (!stream_output(s))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_swf_stream_output_bind_frame_aligned(s, s->converter, frame);
}
