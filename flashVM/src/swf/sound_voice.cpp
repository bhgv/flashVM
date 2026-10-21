/* FlashVM: independent native StartSound snapshots with allocation-free incremental styled reads. */
#include "../core/runtime.hpp"
#include "sound_event_constants.h"
#include "sound_style.hpp"
#include <cstring>
#include <new>

/* [GC:] Own native sample/envelope copies only; no context, managed roots or borrowed SWF bytes survive open. */
struct FVM_SWFSoundVoice
{
    fvm_detail::vector<short> samples; /* Decode one selected source interval once, without repeated copies. */
    fvm_detail::vector<FVM_SWFSoundEnvelope> points; /* Retain ordered independent gain/time records. */
    FVM_SWFSoundVoiceInfo info; /* Retain copied geometry and the next sequential frame position. */
    size_t segment;             /* Cache the current envelope interpolation segment across reads. */
    /* Initialize every progress field before partially fallible preparation. */
    FVM_SWFSoundVoice() : segment(0)
    {
        std::memset(&info, 0, sizeof(info));
        info.struct_size = sizeof(info);
    }
};

/* Compute exact native requested storage; fixed vectors allocate exactly once at their final sizes. */
static FVM_Result voice_charge(size_t frames, unsigned int channels, size_t points, size_t *out)
{
    size_t bytes = sizeof(FVM_SWFSoundVoice);
    if (points > ((size_t)-1 - bytes) / sizeof(FVM_SWFSoundEnvelope))
        return FVM_OUT_OF_MEMORY;
    bytes += points * sizeof(FVM_SWFSoundEnvelope);
    if (frames > ((size_t)-1 - bytes) / sizeof(short) / channels)
        return FVM_OUT_OF_MEMORY;
    *out = bytes + frames * channels * sizeof(short);
    return FVM_OK;
}

/* Capture one coherent event interval within an explicit native-object/PCM/envelope ceiling. */
extern "C" FVM_Result fvm_swf_sound_voice_open_bounded(FVM_Context *context, size_t ordinal,
                                                        size_t budget, FVM_SWFSoundVoice **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    FVM_SWFSoundEvent event;
    event.struct_size = sizeof(event);
    FVM_Result result = fvm_swf_sound_event_at(context, ordinal, &event);
    if (result != FVM_OK)
        return result;
    if (event.flags & FVM_SWF_SOUND_EVENT_STOP)
        return FVM_UNSUPPORTED;
    FVM_SWFSoundInfo source;
    source.struct_size = sizeof(source);
    result = fvm_swf_sound_info(context, event.sound_id, &source);
    if (result != FVM_OK)
        return result;
    unsigned long first = 0;
    if (event.flags & FVM_SWF_SOUND_EVENT_IN_POINT)
    {
        FVM_Result position_result = fvm_swf_sound_position(event.in_point, source.sample_rate, &first);
        if (position_result != FVM_OK)
            return position_result;
    }
    unsigned long end = source.sample_count;
    if (event.flags & FVM_SWF_SOUND_EVENT_OUT_POINT)
    {
        unsigned long last = 0;
        FVM_Result position_result = fvm_swf_sound_position(event.out_point, source.sample_rate, &last);
        if (position_result != FVM_OK)
            return position_result;
        if (last >= source.sample_count)
            return FVM_INVALID_ARGUMENT;
        end = last + 1;
    }
    if (first > end)
        return FVM_INVALID_ARGUMENT;
    unsigned int plays = event.flags & FVM_SWF_SOUND_EVENT_LOOPS ? event.loop_count : FVM_SWF_SOUND_EVENT_DEFAULT_PLAYS;
    size_t checked = 0;
    result = fvm_swf_sound_read_pcm(context, event.sound_id, first, 0, 0, 0, &checked);
    if (result != FVM_OK)
        return result;
    size_t limit = context->runtime->config.memory_limit_bytes ? context->runtime->config.memory_limit_bytes
                                                               : FVM_SWF_DEFAULT_LIMIT;
    size_t envelope_bytes = event.envelope_count * sizeof(FVM_SWFSoundEnvelope);
    unsigned long frames = plays ? end - first : 0;
    if (envelope_bytes > limit || frames > (limit - envelope_bytes) / sizeof(short) / source.channels)
        return FVM_OUT_OF_MEMORY;
    size_t charge = 0;
    result = voice_charge((size_t)frames, source.channels, event.envelope_count, &charge);
    if (result != FVM_OK || charge > budget)
        return FVM_OUT_OF_MEMORY;
    FVM_SWFSoundVoice *voice = 0;
    try
    {
        voice = new FVM_SWFSoundVoice;
        voice->points.resize(event.envelope_count);
        for (size_t i = 0; i < voice->points.size(); ++i)
        {
            voice->points[i].struct_size = sizeof(voice->points[i]);
            result = fvm_swf_sound_event_envelope(context, ordinal, i, &voice->points[i]);
            if (result != FVM_OK || (i && voice->points[i].position_44 <= voice->points[i - 1].position_44))
            {
                delete voice;
                return result != FVM_OK ? result : FVM_INVALID_ARGUMENT;
            }
        }
        voice->samples.resize((size_t)frames * source.channels);
        result = fvm_swf_sound_read_pcm(context, event.sound_id, first, (size_t)frames,
                                        voice->samples.empty() ? 0 : voice->samples.begin(), voice->samples.size(),
                                        &checked);
        if (result != FVM_OK)
        {
            delete voice;
            return result;
        }
        voice->info.sound_id = event.sound_id;
        voice->info.flags = event.flags;
        voice->info.sample_rate = source.sample_rate;
        voice->info.channels = source.channels;
        voice->info.source_first = first;
        voice->info.frames_per_play = (size_t)(end - first);
        voice->info.play_count = plays;
        voice->info.finished = !plays || first == end;
        *out = voice;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete voice;
        return FVM_OUT_OF_MEMORY;
    }
}

/* Preserve the existing runtime payload policy without imposing a new host object ceiling. */
extern "C" FVM_Result fvm_swf_sound_voice_open(FVM_Context *context, size_t ordinal,
                                                FVM_SWFSoundVoice **out)
{
    return fvm_swf_sound_voice_open_bounded(context, ordinal, (size_t)-1, out);
}

/* Inspect only owned exact-capacity vectors; no callback, allocation or source VM is involved. */
extern "C" FVM_Result fvm_swf_sound_voice_storage_info(const FVM_SWFSoundVoice *voice,
                                                        FVM_SWFSoundVoiceStorageInfo *out)
{
    if (!voice || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundVoiceStorageInfo info;
    info.struct_size = sizeof(info);
    info.pcm_bytes = voice->samples.size() * sizeof(short);
    info.envelope_bytes = voice->points.size() * sizeof(FVM_SWFSoundEnvelope);
    info.retained_bytes = sizeof(*voice) + info.pcm_bytes + info.envelope_bytes;
    *out = info;
    return FVM_OK;
}

/* Duplicate immutable PCM/envelopes and current playback progress without entering a source VM. */
extern "C" FVM_Result fvm_swf_sound_voice_clone(const FVM_SWFSoundVoice *source, size_t budget,
                                                 FVM_SWFSoundVoice **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!source)
        return FVM_INVALID_ARGUMENT;
    size_t bytes = 0;
    FVM_Result result = voice_charge(source->samples.size() / source->info.channels,
                                      source->info.channels, source->points.size(), &bytes);
    if (result != FVM_OK || bytes > budget)
        return FVM_OUT_OF_MEMORY;
    FVM_SWFSoundVoice *copy = 0;
    try
    {
        copy = new FVM_SWFSoundVoice;
        copy->points.resize(source->points.size());
        for (size_t i = 0; i < source->points.size(); ++i)
            copy->points[i] = source->points[i];
        copy->samples.resize(source->samples.size());
        if (!source->samples.empty())
            std::memcpy(copy->samples.begin(), source->samples.begin(),
                        source->samples.size() * sizeof(short));
        copy->info = source->info;
        copy->segment = source->segment;
        *out = copy;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete copy;
        return FVM_OUT_OF_MEMORY;
    }
}

/* Copy progress transactionally; snapshot state remains independent of the originating VM. */
extern "C" FVM_Result fvm_swf_sound_voice_info(const FVM_SWFSoundVoice *voice, FVM_SWFSoundVoiceInfo *out)
{
    if (!voice || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    *out = voice->info;
    return FVM_OK;
}

/* Reposition within the immutable selected interval; reconstruct envelope lookup lazily on read. */
extern "C" FVM_Result fvm_swf_sound_voice_seek(FVM_SWFSoundVoice *voice, unsigned int play,
                                                size_t frame)
{
    if (!voice)
        return FVM_INVALID_ARGUMENT;
    bool empty = !voice->info.play_count || !voice->info.frames_per_play;
    if (empty)
    {
        if (play || frame)
            return FVM_INVALID_ARGUMENT;
    }
    else if (play > voice->info.play_count ||
             (play == voice->info.play_count ? frame != 0 : frame >= voice->info.frames_per_play))
        return FVM_INVALID_ARGUMENT;
    voice->info.play_index = play;
    voice->info.frame_in_play = frame;
    voice->info.finished = empty || play == voice->info.play_count;
    voice->segment = 0;
    return FVM_OK;
}

/* Produce a bounded sequential slice, with the same elapsed-time envelope as full rendering. */
extern "C" FVM_Result fvm_swf_sound_voice_read(FVM_SWFSoundVoice *voice, size_t frames, short *out, size_t capacity,
                                               size_t *written)
{
    if (!written)
        return FVM_INVALID_ARGUMENT;
    *written = 0;
    if (!voice || frames > capacity / voice->info.channels || (frames && !out))
        return FVM_INVALID_ARGUMENT;
    if (!frames)
        return FVM_OK;
    if (voice->info.finished)
        return FVM_NOT_FOUND;
    size_t count = 0;
    while (count < frames && !voice->info.finished)
    {
        double elapsed = (double)voice->info.play_index * voice->info.frames_per_play + voice->info.frame_in_play;
        double position = elapsed * FVM_SWF_SOUND_ENVELOPE_RATE / voice->info.sample_rate;
        for (unsigned int channel = 0; channel < voice->info.channels; ++channel)
        {
            double gain = fvm_swf_sound_gain(voice->points, voice->segment, position, channel, voice->info.channels);
            double value = voice->samples[voice->info.frame_in_play * voice->info.channels + channel] * gain /
                           FVM_SWF_SOUND_ENVELOPE_UNITY;
            if (value < FVM_SWF_SOUND_SHORT_MIN)
                value = FVM_SWF_SOUND_SHORT_MIN;
            if (value > FVM_SWF_SOUND_SHORT_MAX)
                value = FVM_SWF_SOUND_SHORT_MAX;
            out[count * voice->info.channels + channel] = (short)value;
        }
        ++count;
        if (++voice->info.frame_in_play == voice->info.frames_per_play)
        {
            voice->info.frame_in_play = 0;
            if (++voice->info.play_index == voice->info.play_count)
                voice->info.finished = 1;
        }
    }
    *written = count * voice->info.channels;
    return FVM_OK;
}

/* [GC:] Free only native snapshot allocations; never consult a context or enter host output. */
extern "C" FVM_Result fvm_swf_sound_voice_close(FVM_SWFSoundVoice *voice)
{
    delete voice;
    return FVM_OK;
}
