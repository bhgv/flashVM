/* FlashVM: bounded PCM producer/retry state and software-device FIFO clock. */
#include "../details/vector.hpp"
#include "sound_output_constants.h"
#include "sound_constants.h"
#include "flashvm/swf_sound_output.h"
#include <cfloat>
#include <cstring>
#include <new>

/* [GC:] No context, managed object or callback root is retained by this native owner. */
struct FVM_SWFSoundOutput
{
    FVM_SWFSoundMixer
        *mixer; /* Borrowed; host may submit events but must not independently read/stop it. */
    FVM_AudioQueueBackend backend; /* Copied callback table with borrowed userdata. */
    fvm_detail::vector<short> pcm; /* One fixed-capacity preparation/retry block. */
    fvm_detail::vector<double>
        identities; /* Optional fixed ID-indexed exclusive end frames on this output epoch. */
    FVM_SWFSoundOutputInfo info;   /* Copied epoch counts and last validated queue state. */
    fvm_detail::vector<short>
        history;           /* [GC:] Optional native accepted-PCM ring; no managed roots. */
    size_t history_cursor; /* Next interleaved frame to overwrite. */
    size_t history_count;  /* Retained complete frames, saturated at ring capacity. */
    size_t release_frames; /* Configured output-tail length; zero keeps legacy allocation/work. */
    size_t block;          /* Maximum frames rendered/accepted by a single pump call. */
    size_t target;         /* Maximum producer-requested queue depth. */
    bool busy;             /* Reject reentry through backend callbacks. */
    /* Initialize ownership before the fallible buffer allocation. */
    FVM_SWFSoundOutput()
        : mixer(0), history_cursor(0), history_count(0), release_frames(0), block(0), target(0),
          busy(false)
    {
        std::memset(&backend, 0, sizeof(backend));
        std::memset(&info, 0, sizeof(info));
        info.struct_size = sizeof(info);
        info.paused = 1;
    }
};

namespace
{
/* Restore callback reentry protection across every result and C++ exception. */
struct BusyScope
{
    FVM_SWFSoundOutput *output; /* Borrow the live native owner for one public operation. */
    explicit BusyScope(FVM_SWFSoundOutput *value) : output(value)
    {
        output->busy = true;
    }
    ~BusyScope()
    {
        output->busy = false;
    }
};

/* Queue consumption is monotonic unless the backend violates exclusive producer/FIFO ownership. */
FVM_Result observe(FVM_SWFSoundOutput *output)
{
    size_t queued = 0;
    FVM_Result status = output->backend.queued(output->backend.userdata, &queued);
    if (status != FVM_OK)
        return status;
    if (queued > output->target || (double)queued > output->info.submitted_frames)
        return FVM_ERROR;
    double played = output->info.submitted_frames - (double)queued;
    if (played < output->info.played_frames)
        return FVM_ERROR;
    output->info.queued_frames = queued;
    output->info.played_frames = played;
    output->info.seconds = played / output->backend.sample_rate;
    output->info.starved = !output->info.paused && output->info.submitted_frames > 0 && !queued;
    return FVM_OK;
}
/* Reset only after acknowledged queue discard; anonymous output release PCM has no sound ID. */
void reset_identities(FVM_SWFSoundOutput *output)
{
    for (size_t i = 0; i < output->identities.size(); ++i)
        output->identities[i] = 0;
}
/* Preserve only accepted blocks; rejected retries must not advance native history. */
void remember(FVM_SWFSoundOutput *output, size_t frames)
{
    if (!output->release_frames)
        return;
    size_t channels = output->backend.channels;
    size_t capacity = output->history.size() / channels;
    for (size_t i = 0; i < frames; ++i)
    {
        for (size_t c = 0; c < channels; ++c)
            output->history[output->history_cursor * channels + c] = output->pcm[i * channels + c];
        output->history_cursor = (output->history_cursor + 1) % capacity;
        if (output->history_count < capacity)
            ++output->history_count;
    }
}
} // namespace

/* Allocate locally before touching backend state; publish only an empty paused queue owner. */
extern "C" FVM_Result fvm_swf_sound_output_create(FVM_SWFSoundMixer *mixer,
                                                  const FVM_AudioQueueBackend *backend,
                                                  size_t block, size_t target,
                                                  FVM_SWFSoundOutput **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!mixer || !backend || backend->struct_size != sizeof(*backend) ||
        backend->abi_version != FVM_AUDIO_QUEUE_ABI_VERSION || !backend->write ||
        !backend->queued || !backend->pause || !backend->clear || !block ||
        block > FVM_SWF_SOUND_OUTPUT_MAX_BLOCK || target < block ||
        target > FVM_SWF_SOUND_OUTPUT_MAX_QUEUE)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixerInfo geometry;
    geometry.struct_size = sizeof(geometry);
    FVM_Result status = fvm_swf_sound_mixer_info(mixer, &geometry);
    if (status != FVM_OK || geometry.sample_rate != backend->sample_rate ||
        geometry.channels != backend->channels)
        return FVM_INVALID_ARGUMENT;
    if (DBL_MANT_DIG < FVM_SWF_SOUND_OUTPUT_CLOCK_BITS)
        return FVM_UNSUPPORTED;
    FVM_SWFSoundOutput *output = 0;
    try
    {
        output = new FVM_SWFSoundOutput;
        output->mixer = mixer;
        output->backend = *backend;
        output->block = block;
        output->target = target;
        output->pcm.resize(block * geometry.channels);
        /* The host grants exclusive control of an empty backend; creation never discards external
         * PCM. */
        status = backend->pause(backend->userdata, 1);
        if (status == FVM_OK)
        {
            size_t queued = 0;
            status = backend->queued(backend->userdata, &queued);
            if (status == FVM_OK && queued)
                status = FVM_INVALID_ARGUMENT;
        }
        if (status != FVM_OK)
        {
            delete output;
            return status;
        }
        *out = output;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete output;
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        delete output;
        return FVM_ERROR;
    }
}

/* Configure native ID deadlines transactionally before any voice/PCM enters this output epoch. */
extern "C" FVM_Result fvm_swf_sound_output_set_identity_tracking(FVM_SWFSoundOutput *output,
                                                                  int enabled)
{
    if (!output || output->busy || (enabled != 0 && enabled != 1) ||
        output->info.submitted_frames || output->info.pending_frames)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixerInfo mixer;
    mixer.struct_size = sizeof(mixer);
    FVM_Result result = fvm_swf_sound_mixer_info(output->mixer, &mixer);
    if (result != FVM_OK || mixer.active_voices)
        return result != FVM_OK ? result : FVM_INVALID_ARGUMENT;
    BusyScope scope(output);
    if (enabled == !output->identities.empty())
        return FVM_OK;
    try
    {
        fvm_detail::vector<double> table;
        if (enabled)
        {
            if (FVM_SWF_SOUND_OUTPUT_ID_SLOTS > (size_t)-1 / sizeof(double))
                return FVM_OUT_OF_MEMORY;
            table.resize(FVM_SWF_SOUND_OUTPUT_ID_SLOTS);
        }
        output->identities.swap(table);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Requested native storage excludes the borrowed mixer/device and allocator bookkeeping. */
extern "C" FVM_Result fvm_swf_sound_output_storage_info(const FVM_SWFSoundOutput *output,
                                                         FVM_SWFSoundOutputStorageInfo *out)
{
    if (!output || output->busy || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundOutputStorageInfo info;
    info.struct_size = sizeof(info);
    info.fixed_bytes = sizeof(*output);
    info.pcm_bytes = output->pcm.size() * sizeof(short);
    info.history_bytes = output->history.size() * sizeof(short);
    info.identity_bytes = output->identities.size() * sizeof(double);
    size_t bytes = info.fixed_bytes;
    const size_t parts[] = {info.pcm_bytes, info.history_bytes, info.identity_bytes};
    for (size_t i = 0; i < sizeof(parts) / sizeof(parts[0]); ++i)
    {
        if (parts[i] > (size_t)-1 - bytes)
            return FVM_OUT_OF_MEMORY;
        bytes += parts[i];
    }
    info.retained_bytes = bytes;
    *out = info;
    return FVM_OK;
}

/* Observe the exclusive FIFO clock, then test one copied native identity deadline. */
extern "C" FVM_Result fvm_swf_sound_output_sound_active(FVM_SWFSoundOutput *output,
                                                         unsigned int id, int *out)
{
    if (!output || output->busy || !out || !id || id > FVM_SWF_SOUND_ID_MAX)
        return FVM_INVALID_ARGUMENT;
    if (output->identities.empty())
        return FVM_UNSUPPORTED;
    BusyScope scope(output);
    try
    {
        FVM_Result result = observe(output);
        if (result == FVM_OK)
            *out = output->identities[id] > output->info.played_frames;
        return result;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Adopt only on the uncomposed side of pending/accepted PCM; retain exact per-ID end clocks. */
extern "C" FVM_Result fvm_swf_sound_output_submit_prepared_bounded(FVM_SWFSoundOutput *output,
                                                       FVM_SWFSoundVoice **voice, size_t budget)
{
    if (!output || output->busy || !voice || !*voice)
        return FVM_INVALID_ARGUMENT;
    if (output->identities.empty())
        return FVM_UNSUPPORTED;
    FVM_SWFSoundVoiceInfo source;
    source.struct_size = sizeof(source);
    FVM_Result result = fvm_swf_sound_voice_info(*voice, &source);
    if (result != FVM_OK)
        return result;
    BusyScope scope(output);
    try
    {
        result = observe(output);
        if (result != FVM_OK)
            return result;
        if ((source.flags & FVM_SWF_SOUND_EVENT_NO_MULTIPLE) &&
            output->identities[source.sound_id] > output->info.played_frames)
        {
            fvm_swf_sound_voice_close(*voice);
            *voice = 0;
            return FVM_OK;
        }
        double duration = 0;
        result = fvm_swf_sound_voice_output_frames(*voice, output->backend.sample_rate, &duration);
        if (result != FVM_OK)
            return result;
        double cursor = output->info.submitted_frames + (double)output->info.pending_frames;
        if (cursor > FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - duration)
            return FVM_UNSUPPORTED;
        double end = cursor + duration;
        result = fvm_swf_sound_mixer_submit_prepared_bounded(output->mixer, voice, budget);
        if (result == FVM_OK && duration && end > output->identities[source.sound_id])
            output->identities[source.sound_id] = end;
        return result;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Stop future same-ID composition and release its NoMultiple lease without editing retained PCM. */
extern "C" FVM_Result fvm_swf_sound_output_stop_id(FVM_SWFSoundOutput *output, unsigned int id)
{
    if (!output || output->busy || !id || id > FVM_SWF_SOUND_ID_MAX)
        return FVM_INVALID_ARGUMENT;
    if (output->identities.empty())
        return FVM_UNSUPPORTED;
    BusyScope scope(output);
    FVM_Result result = fvm_swf_sound_mixer_stop_id(output->mixer, id);
    if (result == FVM_OK)
        output->identities[id] = 0;
    return result;
}

/* Render only when needed; a rejected complete block remains stable until acceptance or stop/close.
 */
extern "C" FVM_Result fvm_swf_sound_output_pump(FVM_SWFSoundOutput *output, size_t *accepted)
{
    if (accepted)
        *accepted = 0;
    if (!output || output->busy)
        return FVM_INVALID_ARGUMENT;
    BusyScope scope(output);
    try
    {
        FVM_Result status = observe(output);
        if (status != FVM_OK)
            return status;
        size_t available = output->target - output->info.queued_frames;
        if (!available || output->info.pending_frames > available)
            return FVM_NOT_FOUND;
        size_t count = output->info.pending_frames;
        if (!count)
        {
            count = available < output->block ? available : output->block;
            if (output->info.submitted_frames > FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - (double)count)
                return FVM_ERROR;
            size_t written = 0;
            status = fvm_swf_sound_mixer_read(output->mixer, count, output->pcm.begin(),
                                              output->pcm.size(), &written);
            if (status != FVM_OK)
                return status;
            output->info.pending_frames = count;
        }
        status = output->backend.write(output->backend.userdata, output->pcm.begin(), count);
        if (status != FVM_OK)
            return status;
        remember(output, count);
        output->info.submitted_frames += count;
        output->info.queued_frames += count;
        output->info.pending_frames = 0;
        output->info.starved = 0;
        if (accepted)
            *accepted = count;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Preserve prepared and queued data while pausing/resuming actual backend consumption. */
extern "C" FVM_Result fvm_swf_sound_output_pause(FVM_SWFSoundOutput *output, int paused)
{
    if (!output || output->busy || (paused != 0 && paused != 1))
        return FVM_INVALID_ARGUMENT;
    BusyScope scope(output);
    try
    {
        FVM_Result status = output->backend.pause(output->backend.userdata, paused);
        if (status == FVM_OK)
            output->info.paused = paused;
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Refresh FIFO observation, publishing no caller output if the backend fails or violates its
 * contract. */
extern "C" FVM_Result fvm_swf_sound_output_info(FVM_SWFSoundOutput *output,
                                                FVM_SWFSoundOutputInfo *info)
{
    if (!output || output->busy || !info || info->struct_size != sizeof(*info))
        return FVM_INVALID_ARGUMENT;
    BusyScope scope(output);
    try
    {
        FVM_Result status = observe(output);
        if (status == FVM_OK)
            *info = output->info;
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Reset only after successful queue discard; a failed clear keeps pending samples/voices for
 * recovery. */
extern "C" FVM_Result fvm_swf_sound_output_stop(FVM_SWFSoundOutput *output)
{
    if (!output || output->busy)
        return FVM_INVALID_ARGUMENT;
    BusyScope scope(output);
    try
    {
        FVM_Result status = output->backend.pause(output->backend.userdata, 1);
        if (status != FVM_OK)
            return status;
        output->info.paused = 1;
        status = output->backend.clear(output->backend.userdata);
        if (status != FVM_OK)
            return status;
        fvm_swf_sound_mixer_stop_all(output->mixer);
        reset_identities(output);
        output->history_cursor = output->history_count = 0;
        std::memset(&output->info, 0, sizeof(output->info));
        output->info.struct_size = sizeof(output->info);
        output->info.paused = 1;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Configure optional history transactionally outside playback; no source or backend mutation. */
extern "C" FVM_Result fvm_swf_sound_output_set_release(FVM_SWFSoundOutput *output, size_t frames)
{
    if (!output || output->busy || output->info.submitted_frames != 0 ||
        (frames && (frames < FVM_OUTPUT_RELEASE_MIN_FRAMES || frames > output->block)))
        return FVM_INVALID_ARGUMENT;
    BusyScope scope(output);
    try
    {
        fvm_detail::vector<short> history;
        if (frames)
            history.resize((output->target + output->block) * output->backend.channels);
        output->history.swap(history);
        output->history_cursor = output->history_count = 0;
        output->release_frames = frames;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Keep a running backend running: clearing must not itself insert a paused silent device block. */
extern "C" FVM_Result fvm_swf_sound_output_stop_release(FVM_SWFSoundOutput *output)
{
    if (!output || output->busy || !output->release_frames)
        return FVM_INVALID_ARGUMENT;
    BusyScope scope(output);
    try
    {
        FVM_Result status = observe(output);
        if (status != FVM_OK)
            return status;
        short seed[FVM_OUTPUT_RELEASE_CHANNELS] = {0, 0};
        unsigned int channels = output->backend.channels;
        size_t queued = output->info.queued_frames;
        if (!output->info.paused && output->history_count > queued)
        {
            size_t capacity = output->history.size() / channels;
            size_t at = (output->history_cursor + capacity - queued - 1) % capacity;
            for (unsigned int c = 0; c < channels; ++c)
                seed[c] = output->history[at * channels + c];
        }
        status = output->backend.clear(output->backend.userdata);
        if (status != FVM_OK)
            return status; /* Pending PCM and source ownership remain available for explicit retry.
                            */
        int paused = output->info.paused;
        fvm_swf_sound_mixer_stop_all(output->mixer);
        reset_identities(output);
        std::memset(&output->info, 0, sizeof(output->info));
        output->info.struct_size = sizeof(output->info);
        output->info.paused = paused;
        output->history_cursor = output->history_count = 0;
        if (!seed[0] && !seed[1])
            return FVM_OK;
        size_t count = output->release_frames;
        for (size_t i = 0; i < count; ++i)
            for (unsigned int c = 0; c < channels; ++c)
                output->pcm[i * channels + c] =
                    (short)((double)seed[c] * (count - 1 - i) / (count - 1));
        output->info.pending_frames = count;
        status = output->backend.write(output->backend.userdata, output->pcm.begin(), count);
        if (status != FVM_OK)
            return status; /* Old audio is already discarded; preserve the exact release block for
                              pump retry. */
        remember(output, count);
        output->info.submitted_frames = (double)count;
        output->info.queued_frames = count;
        output->info.pending_frames = 0;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Release producer state even after an unrecoverable device error; copied backend PCM stays
 * device-owned. */
extern "C" void fvm_swf_sound_output_close(FVM_SWFSoundOutput *output)
{
    if (output && output->busy)
        return;
    delete output;
}
