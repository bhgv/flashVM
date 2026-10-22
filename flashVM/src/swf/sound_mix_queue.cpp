/* Native composition backend: one voice advance, exact retained mixed retry, no hot-path
 * allocation. */
#include "flashvm/swf_sound_mix_queue.h"
#include "../details/vector.hpp"
#include "sound_mix_queue_constants.h"
#include <cstring>
#include <new>

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
    size_t capacity; /* Immutable maximum interleaved frames. */
    size_t pending;  /* Zero or initialized mixed frames whose voice time is already consumed. */
    bool tail; /* Pending block was prepared by the event-only tail pump. */
    bool busy;       /* Reject callback reentry, independent of external thread synchronization. */
    /* Initialize callback state before any fallible vector acquisition. */
    FVM_SWFSoundMixQueue() : mixer(0), capacity(0), pending(0), tail(false), busy(false)
    {
    }
};
namespace
{
/* Compute requested native bytes before any storage allocation; exclude allocator overhead. */
FVM_Result storage_bytes(unsigned int channels, size_t block, FVM_SWFSoundMixQueueStorageInfo &out)
{
    if ((channels != 1 && channels != 2) || !block || block > FVM_SWF_SOUND_OUTPUT_MAX_BLOCK)
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
/* Forward only acknowledged queue counts; preserve caller output after callback failure. */
FVM_Result observe(void *raw, size_t *out)
{
    FVM_SWFSoundMixQueue *queue = static_cast<FVM_SWFSoundMixQueue *>(raw);
    if (!queue || queue->busy || !out)
        return FVM_INVALID_ARGUMENT;
    MixBusyScope scope(queue);
    try
    {
        size_t frames = 0;
        FVM_Result result = queue->backend.queued(queue->backend.userdata, &frames);
        if (result == FVM_OK)
            *out = frames;
        return result;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
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
            queue->pending = 0;
            queue->tail = false;
        }
        return result;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
} // namespace
/* Prepare complete native storage without touching the real queue or publishing partial ownership.
 */
extern "C" FVM_Result fvm_swf_sound_mix_queue_create(FVM_SWFSoundMixer *mixer,
                                                     const FVM_AudioQueueBackend *backend,
                                                     size_t block, FVM_SWFSoundMixQueue **out,
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
    if (block > (size_t)-1 / info.channels / sizeof(short))
        return FVM_OUT_OF_MEMORY;
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

/* Observe fixed owner and array requests; pending state never grows native storage. */
extern "C" FVM_Result fvm_swf_sound_mix_queue_storage_info(const FVM_SWFSoundMixQueue *queue,
                                                           FVM_SWFSoundMixQueueStorageInfo *out)
{
    if (!queue || queue->busy || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    return storage_bytes(queue->backend.channels, queue->capacity, *out);
}

/* [GC:] Native-only cleanup after producer quiescence; neither borrowed owner is released. */
extern "C" void fvm_swf_sound_mix_queue_close(FVM_SWFSoundMixQueue *queue)
{
    delete queue;
}
