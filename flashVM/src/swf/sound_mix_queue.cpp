/* Native composition backend: one voice advance, exact retained mixed retry, no hot-path
 * allocation. */
#include "flashvm/swf_sound_mix_queue.h"
#include "../details/vector.hpp"
#include "sound_mix_queue_constants.h"
#include "sound_mix_queue_internal.h"
#include "sound_constants.h"
#include <cstring>
#include <new>
#include <cfloat>

/* No VM or managed reference is retained; all mutable access requires external serialization. */
struct FVM_SWFSoundMixQueue
{
    FVM_SWFSoundMixer
        *mixer; /* Borrowed exclusively for rendering; host may serialize submissions. */
    FVM_AudioQueueBackend backend; /* Copied real device callbacks and borrowed userdata. */
    fvm_detail::vector<short>
        original; /* Fixed original-input witness for pending retry matching. */
    fvm_detail::vector<short>
        mixed;       /* Fixed actual mixed block; backend must copy before returning OK. */
    fvm_detail::vector<double> identities; /* Optional fixed per-ID exclusive dequeue deadlines. */
    size_t capacity; /* Immutable maximum interleaved frames. */
    size_t pending;  /* Zero or initialized mixed frames whose voice time is already consumed. */
    bool clock_active; /* Explicit opt-in; legacy callback-only owners retain prior behavior. */
    double clock_submitted; /* Successful real writes in the current shared frame epoch. */
    double clock_played; /* Last valid monotone accepted-minus-queued observation. */
    bool tail; /* Pending block was prepared by the event-only tail pump. */
    bool busy;       /* Reject callback reentry, independent of external thread synchronization. */
    /* Initialize callback state before any fallible vector acquisition. */
    FVM_SWFSoundMixQueue() : mixer(0), capacity(0), pending(0), clock_active(false), clock_submitted(0), clock_played(0), tail(false), busy(false)
    {
    }
};
namespace
{
/* Compute requested native bytes before any storage allocation; exclude allocator overhead. */
FVM_Result storage_bytes(unsigned int channels, size_t block, FVM_SWFSoundMixQueueStorageInfo &out)
{
    if ((channels < FVM_SWF_SOUND_MIX_QUEUE_MIN_CHANNELS ||
         channels > FVM_SWF_SOUND_MIX_QUEUE_MAX_CHANNELS) || !block || block > FVM_SWF_SOUND_OUTPUT_MAX_BLOCK)
        return FVM_INVALID_ARGUMENT;
    if (block > (size_t)-1 / channels / sizeof(short) / FVM_SWF_SOUND_MIX_QUEUE_BUFFERS)
        return FVM_OUT_OF_MEMORY;
    size_t buffers = block * channels * sizeof(short) * FVM_SWF_SOUND_MIX_QUEUE_BUFFERS;
    if (buffers > (size_t)-1 - sizeof(FVM_SWFSoundMixQueue)) return FVM_OUT_OF_MEMORY;
    FVM_SWFSoundMixQueueStorageInfo info;
    info.struct_size = sizeof(info);
    info.fixed_bytes = sizeof(FVM_SWFSoundMixQueue);
    info.buffer_bytes = buffers;
    info.retained_bytes = info.fixed_bytes + buffers;
    out = info;
    return FVM_OK;
}

/* Include optional deadline storage while retaining the public fixed+buffer sum identity. */
FVM_Result storage_with_identities(unsigned int channels, size_t block, size_t slots,
                                    FVM_SWFSoundMixQueueStorageInfo &out)
{
    FVM_SWFSoundMixQueueStorageInfo info;
    FVM_Result result = storage_bytes(channels, block, info);
    if (result != FVM_OK) return result;
    if (slots > (size_t)-1 / sizeof(double)) return FVM_OUT_OF_MEMORY;
    size_t bytes = slots * sizeof(double);
    if (bytes > (size_t)-1 - info.retained_bytes) return FVM_OUT_OF_MEMORY;
    info.buffer_bytes += bytes;
    info.retained_bytes += bytes;
    out = info;
    return FVM_OK;
}

/* Restore reentry protection even when a C++ backend violates the no-throw C boundary. */
struct MixBusyScope
{
    FVM_SWFSoundMixQueue *queue; /* Borrow for exactly one externally serialized callback. */
    /* Mark one serial operation busy before invoking any borrowed backend callback. */
    explicit MixBusyScope(FVM_SWFSoundMixQueue *value) : queue(value)
    {
        queue->busy = true;
    }
    /* Release recursion protection on normal, error and exception exits. */
    ~MixBusyScope()
    {
        queue->busy = false;
    }
};
/* Validate new input or match its complete original witness; never remix a rejected block. */
FVM_Result write_composed(void *raw, const short *pcm, size_t frames, bool tail)
{
    FVM_SWFSoundMixQueue *queue = static_cast<FVM_SWFSoundMixQueue *>(raw);
    if (!queue || queue->busy || frames > queue->capacity || (frames && !pcm && !tail))
        return FVM_INVALID_ARGUMENT;
    if (!frames)
        return queue->pending ? FVM_INVALID_ARGUMENT : FVM_OK;
    if (queue->clock_active &&
        queue->clock_submitted > FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - (double)frames)
        return FVM_UNSUPPORTED;
    MixBusyScope scope(queue);
    try
    {
        size_t values = frames * queue->backend.channels;
        size_t bytes = values * sizeof(short);
        if (queue->pending)
        {
            if (tail != queue->tail || frames != queue->pending ||
                (!tail && std::memcmp(pcm, queue->original.begin(), bytes)))
                return FVM_INVALID_ARGUMENT;
        }
        else
        {
            if (tail)
            {
                std::memset(queue->original.begin(), 0, bytes);
                std::memset(queue->mixed.begin(), 0, bytes);
            }
            else
            {
                std::memcpy(queue->original.begin(), pcm, bytes);
                std::memcpy(queue->mixed.begin(), pcm, bytes);
            }
            size_t written = 0;
            FVM_Result result = fvm_swf_sound_mixer_add(queue->mixer, frames, queue->mixed.begin(),
                                                        queue->mixed.size(), &written);
            if (result != FVM_OK)
                return result;
            if (written != values)
                return FVM_ERROR;
            queue->pending = frames;
            queue->tail = tail;
        }
        FVM_Result result =
            queue->backend.write(queue->backend.userdata, queue->mixed.begin(), frames);
        if (result == FVM_OK)
        {
            queue->pending = 0;
            queue->tail = false;
            if (queue->clock_active) queue->clock_submitted += (double)frames;
        }
        return result;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
/* Stream writes retain their original byte witness and cannot consume a tail retry. */
FVM_Result write_mixed(void *raw, const short *pcm, size_t frames)
{
    return write_composed(raw, pcm, frames, false);
}
/* Observe while the public operation already holds the native callback reentry guard. */
FVM_Result observe_locked(FVM_SWFSoundMixQueue *queue, size_t *out)
{
    try
    {
        size_t frames = 0;
        FVM_Result result = queue->backend.queued(queue->backend.userdata, &frames);
        if (result == FVM_OK)
        {
            if (queue->clock_active)
            {
                if (frames > FVM_SWF_SOUND_OUTPUT_MAX_QUEUE ||
                    (double)frames > queue->clock_submitted) return FVM_ERROR;
                double played = queue->clock_submitted - (double)frames;
                if (played < queue->clock_played) return FVM_ERROR;
                queue->clock_played = played;
            }
            *out = frames;
        }
        return result;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
/* Forward acknowledged counts with one externally serialized callback reentry scope. */
FVM_Result observe(void *raw, size_t *out)
{
    FVM_SWFSoundMixQueue *queue = static_cast<FVM_SWFSoundMixQueue *>(raw);
    if (!queue || queue->busy || !out) return FVM_INVALID_ARGUMENT;
    MixBusyScope scope(queue);
    return observe_locked(queue, out);
}
/* Pause never changes pending ownership or consumes voice time. */
FVM_Result pause_queue(void *raw, int paused)
{
    FVM_SWFSoundMixQueue *queue = static_cast<FVM_SWFSoundMixQueue *>(raw);
    if (!queue || queue->busy || (paused != 0 && paused != 1))
        return FVM_INVALID_ARGUMENT;
    MixBusyScope scope(queue);
    try
    {
        return queue->backend.pause(queue->backend.userdata, paused);
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
/* Clear cancels the retry only after device acknowledgement; it never rewinds event voices. */
FVM_Result clear_queue(void *raw)
{
    FVM_SWFSoundMixQueue *queue = static_cast<FVM_SWFSoundMixQueue *>(raw);
    if (!queue || queue->busy)
        return FVM_INVALID_ARGUMENT;
    MixBusyScope scope(queue);
    try
    {
        FVM_Result result = queue->backend.clear(queue->backend.userdata);
        if (result == FVM_OK)
        {
            if (!queue->identities.empty())
            {
                /* Live mixer ownership is a precondition: discard all tracked future voices. */
                (void)fvm_swf_sound_mixer_stop_all(queue->mixer);
                for (size_t i = 0; i < queue->identities.size(); ++i) queue->identities[i] = 0;
            }
            queue->pending = 0;
            queue->tail = false;
            if (queue->clock_active)
                queue->clock_submitted = queue->clock_played = 0;
        }
        return result;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
} // namespace
/* Pair only the exact live adapted table; arbitrary userdata aliases are not sufficient. */
bool fvm_swf_sound_mix_queue_matches(const FVM_SWFSoundMixQueue *queue,
                                      const FVM_AudioQueueBackend *backend)
{
    return queue && !queue->busy && backend &&
        backend->struct_size == sizeof(*backend) &&
        backend->abi_version == FVM_AUDIO_QUEUE_ABI_VERSION &&
        backend->userdata == queue && backend->write == write_mixed &&
        backend->queued == observe && backend->pause == pause_queue &&
        backend->clear == clear_queue && backend->sample_rate == queue->backend.sample_rate &&
        backend->channels == queue->backend.channels;
}

/* Prepare complete native storage without touching the real queue or publishing partial ownership.
 */
extern "C" FVM_Result fvm_swf_sound_mix_queue_create_bounded(FVM_SWFSoundMixer *mixer,
                                                     const FVM_AudioQueueBackend *backend,
                                                     size_t block, size_t budget, FVM_SWFSoundMixQueue **out,
                                                     FVM_AudioQueueBackend *adapted)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!adapted || !backend || backend->struct_size != sizeof(*backend) ||
        backend->abi_version != FVM_AUDIO_QUEUE_ABI_VERSION || !backend->write ||
        !backend->queued || !backend->pause || !backend->clear || !block ||
        block > FVM_SWF_SOUND_OUTPUT_MAX_BLOCK)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixerInfo info;
    info.struct_size = sizeof(info);
    FVM_Result result = fvm_swf_sound_mixer_info(mixer, &info);
    if (result != FVM_OK)
        return result;
    if (backend->sample_rate != info.sample_rate || backend->channels != info.channels)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixQueueStorageInfo storage;
    result = storage_bytes(info.channels, block, storage);
    if (result != FVM_OK) return result;
    if (storage.retained_bytes > budget) return FVM_OUT_OF_MEMORY;
    FVM_SWFSoundMixQueue *queue = 0;
    try
    {
        queue = new FVM_SWFSoundMixQueue;
        queue->backend = *backend;
        queue->mixer = mixer;
        queue->capacity = block;
        queue->original.resize(block * info.channels);
        queue->mixed.resize(block * info.channels);
        FVM_AudioQueueBackend table = *backend;
        table.userdata = queue;
        table.write = write_mixed;
        table.queued = observe;
        table.pause = pause_queue;
        table.clear = clear_queue;
        *adapted = table;
        *out = queue;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete queue;
        return FVM_OUT_OF_MEMORY;
    }
}
/* Preserve the original acquisition ABI with an unrestricted native-request ceiling. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_create(FVM_SWFSoundMixer *mixer,
                                                     const FVM_AudioQueueBackend *backend,
                                                     size_t block, FVM_SWFSoundMixQueue **out,
                                                     FVM_AudioQueueBackend *adapted)
{
    return fvm_swf_sound_mix_queue_create_bounded(mixer, backend, block, (size_t)-1, out, adapted);
}

/* Authorize one event-only block after the host has quiesced its base producer. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_pump_tail(FVM_SWFSoundMixQueue *queue,
                                                       size_t frames, size_t target,
                                                       size_t *accepted)
{
    if (accepted) *accepted = 0;
    if (!queue || queue->busy || !frames || frames > queue->capacity || target < frames ||
        target > FVM_SWF_SOUND_OUTPUT_MAX_QUEUE ||
        (queue->pending && (!queue->tail || queue->pending != frames)))
        return FVM_INVALID_ARGUMENT;
    if (!queue->pending)
    {
        FVM_SWFSoundMixerInfo info;
        info.struct_size = sizeof(info);
        FVM_Result result = fvm_swf_sound_mixer_info(queue->mixer, &info);
        if (result != FVM_OK) return result;
        if (!info.active_voices) return FVM_NOT_FOUND;
    }
    size_t queued = 0;
    FVM_Result result = observe(queue, &queued);
    if (result != FVM_OK) return result;
    if (queued > FVM_SWF_SOUND_OUTPUT_MAX_QUEUE) return FVM_INVALID_ARGUMENT;
    if (queued > target || frames > target - queued) return FVM_NOT_FOUND;
    result = write_composed(queue, 0, frames, true);
    if (result == FVM_OK && accepted) *accepted = frames;
    return result;
}

/* Explicitly begin a fresh shared epoch; do not infer a queue baseline at construction. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_clock_begin(FVM_SWFSoundMixQueue *queue)
{
    if (!queue || queue->busy || queue->pending || !queue->identities.empty())
        return FVM_INVALID_ARGUMENT;
    if (DBL_MANT_DIG < FVM_SWF_SOUND_OUTPUT_CLOCK_BITS) return FVM_UNSUPPORTED;
    MixBusyScope scope(queue);
    try
    {
        size_t queued = 0;
        FVM_Result result = queue->backend.queued(queue->backend.userdata, &queued);
        if (result != FVM_OK) return result;
        if (queued) return FVM_INVALID_ARGUMENT;
        queue->clock_submitted = queue->clock_played = 0;
        queue->clock_active = true;
        return FVM_OK;
    }
    catch (...) { return FVM_ERROR; }
}

/* Relinquish accounting only after acknowledged retry ownership and backlog are empty. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_clock_end(FVM_SWFSoundMixQueue *queue)
{
    if (!queue || queue->busy || queue->pending || !queue->identities.empty())
        return FVM_INVALID_ARGUMENT;
    if (!queue->clock_active) return FVM_NOT_FOUND;
    size_t queued = 0;
    FVM_Result result = observe(queue, &queued);
    if (result != FVM_OK) return result;
    if (queued) return FVM_NOT_FOUND;
    queue->clock_active = false;
    return FVM_OK;
}

/* Publish only a freshly validated base-plus-tail accepted-minus-queued observation. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_clock_info(FVM_SWFSoundMixQueue *queue,
                                                         FVM_SWFSoundMixQueueClock *out)
{
    if (!queue || queue->busy || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    if (!queue->clock_active) return FVM_NOT_FOUND;
    size_t queued = 0;
    FVM_Result result = observe(queue, &queued);
    if (result != FVM_OK) return result;
    FVM_SWFSoundMixQueueClock info;
    info.struct_size = sizeof(info);
    info.submitted_frames = queue->clock_submitted;
    info.played_frames = queue->clock_played;
    info.seconds = info.played_frames / queue->backend.sample_rate;
    info.queued_frames = queued;
    *out = info;
    return FVM_OK;
}

/* Inspect retry ownership without polling the device or using source VM state. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_info(const FVM_SWFSoundMixQueue *queue,
                                                   FVM_SWFSoundMixQueueInfo *out)
{
    if (!queue || queue->busy || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixQueueInfo info;
    info.struct_size = sizeof(info);
    info.capacity_frames = queue->capacity;
    info.pending_frames = queue->pending;
    *out = info;
    return FVM_OK;
}
/* Expose input ownership independently of any device queue or playback observation. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_pending(const FVM_SWFSoundMixQueue *queue,
                                                      FVM_SWFSoundMixQueuePending *out)
{
    if (!queue || queue->busy || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixQueuePending info;
    info.struct_size = sizeof(info);
    info.frames = queue->pending;
    info.origin = !queue->pending ? FVM_SWF_SOUND_MIX_QUEUE_PENDING_NONE :
                  queue->tail ? FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL :
                                FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE;
    *out = info;
    return FVM_OK;
}

/* Predict exact adapter-owned allocation requests independently of mixer/device acquisition. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_storage_required(unsigned int channels,
                                                               size_t block, size_t *out)
{
    if (!out) return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixQueueStorageInfo info;
    FVM_Result result = storage_bytes(channels, block, info);
    if (result == FVM_OK) *out = info.retained_bytes;
    return result;
}

/* Predict optional deadline allocation with the same checked accounting as mode admission. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_storage_required_for_tracking(
    unsigned int channels, size_t block, int tracking, size_t *out)
{
    if (!out || (tracking != 0 && tracking != 1)) return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixQueueStorageInfo info;
    FVM_Result result = storage_with_identities(channels, block,
        tracking ? FVM_SWF_SOUND_MIX_QUEUE_ID_SLOTS : 0, info);
    if (result == FVM_OK) *out = info.retained_bytes;
    return result;
}

/* Observe fixed owner and array requests; pending state never grows native storage. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_storage_info(const FVM_SWFSoundMixQueue *queue,
                                                           FVM_SWFSoundMixQueueStorageInfo *out)
{
    if (!queue || queue->busy || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    return storage_with_identities(queue->backend.channels, queue->capacity, queue->identities.size(), *out);
}

/* Allocate optional deadlines only at an explicit clock/voice safe point. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_set_identity_tracking(FVM_SWFSoundMixQueue *queue,
                                                                    int enabled, size_t budget)
{
    if (!queue || queue->busy || (enabled != 0 && enabled != 1)) return FVM_INVALID_ARGUMENT;
    if ((enabled != 0) == !queue->identities.empty()) return FVM_OK;
    if (!queue->clock_active || queue->pending) return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixerInfo mixer;
    mixer.struct_size = sizeof(mixer);
    FVM_Result result = fvm_swf_sound_mixer_info(queue->mixer, &mixer);
    if (result != FVM_OK) return result;
    if (mixer.active_voices) return FVM_INVALID_ARGUMENT;
    if (enabled && (queue->clock_submitted || queue->clock_played)) return FVM_INVALID_ARGUMENT;
    if (!enabled)
    {
        if (queue->clock_played != queue->clock_submitted) return FVM_INVALID_ARGUMENT;
        for (size_t i = 0; i < queue->identities.size(); ++i)
            if (queue->identities[i] > queue->clock_played) return FVM_INVALID_ARGUMENT;
    }
    FVM_SWFSoundMixQueueStorageInfo storage;
    result = storage_with_identities(queue->backend.channels, queue->capacity,
                enabled ? FVM_SWF_SOUND_MIX_QUEUE_ID_SLOTS : 0, storage);
    if (result != FVM_OK) return result;
    if (enabled && storage.retained_bytes > budget) return FVM_OUT_OF_MEMORY;
    MixBusyScope scope(queue);
    try
    {
        fvm_detail::vector<double> table;
        if (enabled) table.resize(FVM_SWF_SOUND_MIX_QUEUE_ID_SLOTS);
        queue->identities.swap(table);
        return FVM_OK;
    }
    catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; }
}

/* Track actual future composition from the far side of every already rendered retry. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_submit_prepared_bounded(FVM_SWFSoundMixQueue *queue,
                                                                      FVM_SWFSoundVoice **voice,
                                                                      size_t budget)
{
    if (!queue || queue->busy || !voice || !*voice) return FVM_INVALID_ARGUMENT;
    if (queue->identities.empty()) return FVM_UNSUPPORTED;
    FVM_SWFSoundVoiceInfo source;
    source.struct_size = sizeof(source);
    FVM_Result result = fvm_swf_sound_voice_info(*voice, &source);
    if (result != FVM_OK) return result;
    if (!source.sound_id || source.sound_id > FVM_SWF_SOUND_ID_MAX) return FVM_INVALID_ARGUMENT;
    MixBusyScope scope(queue);
    try
    {
        size_t queued = 0;
        result = observe_locked(queue, &queued);
        if (result != FVM_OK) return result;
        if ((source.flags & FVM_SWF_SOUND_EVENT_NO_MULTIPLE) &&
            queue->identities[source.sound_id] > queue->clock_played)
        {
            fvm_swf_sound_voice_close(*voice);
            *voice = 0;
            return FVM_OK;
        }
        double duration = 0;
        result = fvm_swf_sound_voice_output_frames(*voice, queue->backend.sample_rate, &duration);
        if (result != FVM_OK) return result;
        if (queue->clock_submitted > FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - (double)queue->pending)
            return FVM_UNSUPPORTED;
        double start = queue->clock_submitted + (double)queue->pending;
        if (start > FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - duration) return FVM_UNSUPPORTED;
        double end = start + duration;
        result = fvm_swf_sound_mixer_submit_prepared_bounded(queue->mixer, voice, budget);
        if (result == FVM_OK && duration && end > queue->identities[source.sound_id])
            queue->identities[source.sound_id] = end;
        return result;
    }
    catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; }
    catch (...) { return FVM_ERROR; }
}

/* Publish exact per-ID deadlines only after one valid shared FIFO observation. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_identity_info(FVM_SWFSoundMixQueue *queue,
                                       unsigned int id, FVM_SWFSoundMixQueueIdentityInfo *out)
{
    if (!queue || queue->busy || !out || out->struct_size != sizeof(*out) ||
        !id || id > FVM_SWF_SOUND_ID_MAX) return FVM_INVALID_ARGUMENT;
    if (queue->identities.empty()) return FVM_UNSUPPORTED;
    MixBusyScope scope(queue);
    size_t queued = 0;
    FVM_Result result = observe_locked(queue, &queued);
    if (result != FVM_OK) return result;
    FVM_SWFSoundMixQueueIdentityInfo info;
    info.struct_size = sizeof(info);
    info.sound_id = id;
    info.played_frames = queue->clock_played;
    info.end_frame = queue->identities[id];
    info.until_end_frames = info.end_frame > info.played_frames ? info.end_frame - info.played_frames : 0;
    info.active = info.until_end_frames > 0;
    *out = info;
    return FVM_OK;
}

/* Keep the simple query on the same validated deadline path as copied diagnostics. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_sound_active(FVM_SWFSoundMixQueue *queue,
                                                           unsigned int id, int *out)
{
    if (!out) return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixQueueIdentityInfo info;
    info.struct_size = sizeof(info);
    FVM_Result result = fvm_swf_sound_mix_queue_identity_info(queue, id, &info);
    if (result == FVM_OK) *out = info.active;
    return result;
}

/* Cancel a future identity lease without editing already accepted or retained mixed bytes. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_stop_id(FVM_SWFSoundMixQueue *queue, unsigned int id)
{
    if (!queue || queue->busy || !id || id > FVM_SWF_SOUND_ID_MAX) return FVM_INVALID_ARGUMENT;
    if (queue->identities.empty()) return FVM_UNSUPPORTED;
    MixBusyScope scope(queue);
    FVM_Result result = fvm_swf_sound_mixer_stop_id(queue->mixer, id);
    if (result == FVM_OK) queue->identities[id] = 0;
    return result;
}

/* Retire every future event contribution without destroying base/pending/accepted PCM. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_stop_all(FVM_SWFSoundMixQueue *queue)
{
    if (!queue || queue->busy) return FVM_INVALID_ARGUMENT;
    if (queue->identities.empty()) return FVM_UNSUPPORTED;
    MixBusyScope scope(queue);
    FVM_Result result = fvm_swf_sound_mixer_stop_all(queue->mixer);
    if (result == FVM_OK)
        for (size_t i = 0; i < queue->identities.size(); ++i) queue->identities[i] = 0;
    return result;
}

/* [GC:] Native-only cleanup after producer quiescence; neither borrowed owner is released. */
extern "C" void fvm_swf_sound_mix_queue_close(FVM_SWFSoundMixQueue *queue)
{
    delete queue;
}
