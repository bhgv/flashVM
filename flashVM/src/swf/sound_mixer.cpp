/* FlashVM: independent native event voices, explicit control delivery and saturated PCM mixing. */
#include "flashvm/swf_sound_mixer.h"
#include "sound_mixer_constants.h"
#include "sound_constants.h"
#include "../details/vector.hpp"
#include <climits>
#include <cstring>
#include <new>

/* One occupied slot owns one immutable-source voice with independent playback progress. */
struct FVM_SWFSoundMixerSlot
{
    FVM_SWFSoundVoice *voice; /* Owned snapshot; null identifies a reusable slot. */
    unsigned int sound_id;    /* Source dictionary ID used by stop and SyncNoMultiple. */
    unsigned int source_rate; /* Source clock from the prepared voice, unchanged by conversion. */
    unsigned int source_channels; /* One or two source channels used by the frame reader. */
    unsigned long
        phase; /* Exact fractional source position, numerator with output-rate denominator. */
    size_t release_frames; /* Total opt-in stop-by-ID release duration; zero means normal voice. */
    size_t release_left;   /* Remaining output frames including the zero endpoint. */
    bool converted;        /* Use rational conversion only for mismatched source/output geometry. */
    bool ready;            /* Left/right lookahead has been initialized by a validated read. */
    bool left_valid;  /* A current source frame remains audible even after voice lookahead reaches
                         EOF. */
    bool right_valid; /* The next source frame exists; false holds the left value to the end. */
    long left[FVM_SWF_SOUND_MIXER_CHANNELS];  /* Channel-mapped current styled source frame. */
    long right[FVM_SWF_SOUND_MIXER_CHANNELS]; /* Channel-mapped next styled source frame. */
    /* Make construction/partial allocation cleanup safe before accepting any events. */
    FVM_SWFSoundMixerSlot()
        : voice(0), sound_id(0), source_rate(0), source_channels(0), phase(0), release_frames(0),
          release_left(0), converted(false), ready(false), left_valid(false), right_valid(false)
    {
    }
};

/* [GC:] Store native owners only: no context pointer, managed root or borrowed SWF data. */
struct FVM_SWFSoundMixer
{
    FVM_SWFSoundMixerInfo info; /* Fixed geometry and exact occupied slot count. */
    FVM_SWFSoundMixerConfig
        config; /* Immutable copied output policy, independent of legacy info ABI. */
    fvm_detail::vector<FVM_SWFSoundMixerSlot>
        slots; /* Allocate the complete slot table on create. */
    short source[FVM_SWF_SOUND_MIXER_CHUNK *
                 FVM_SWF_SOUND_MIXER_CHANNELS]; /* One sequential voice slice. */
    long sum[FVM_SWF_SOUND_MIXER_CHUNK *
             FVM_SWF_SOUND_MIXER_CHANNELS]; /* Unclipped sum of all active slices. */
    /* Zero publication state before any fallible slot allocation. */
    FVM_SWFSoundMixer()
    {
        std::memset(&info, 0, sizeof(info));
        info.struct_size = sizeof(info);
        std::memset(&config, 0, sizeof(config));
        config.struct_size = sizeof(config);
    }
    /* [GC:] Native snapshots remain releasable after every source VM has disappeared. */
    ~FVM_SWFSoundMixer()
    {
        for (size_t i = 0; i < slots.size(); ++i)
            fvm_swf_sound_voice_close(slots[i].voice);
    }
};

/* Accept only nominal rates supported by the existing SWF sound decoder. */
static bool mixer_rate(unsigned int rate)
{
    return rate == FVM_SWF_SOUND_RATE_5K || rate == FVM_SWF_SOUND_RATE_11K ||
           rate == FVM_SWF_SOUND_RATE_22K || rate == FVM_SWF_SOUND_RATE_44K;
}

/* Retire exactly one occupied slot, releasing native PCM/envelope storage without allocation. */
static void retire(FVM_SWFSoundMixer *mixer, size_t slot)
{
    if (mixer->slots[slot].voice)
    {
        fvm_swf_sound_voice_close(mixer->slots[slot].voice);
        mixer->slots[slot].voice = 0;
        --mixer->info.active_voices;
    }
}

/* Preserve legacy strict geometry; conversion requires the separate configured entry point. */
extern "C" FVM_Result fvm_swf_sound_mixer_create(unsigned int rate, unsigned int channels,
                                                 unsigned int capacity, FVM_SWFSoundMixer **out)
{
    FVM_SWFSoundMixerConfig config;
    config.struct_size = sizeof(config);
    config.sample_rate = rate;
    config.channels = channels;
    config.voice_capacity = capacity;
    config.flags = 0;
    return fvm_swf_sound_mixer_create_configured(&config, out);
}

/* Create a bounded owner; the policy and slot table are complete before any handle is published. */
extern "C" FVM_Result fvm_swf_sound_mixer_create_bounded(const FVM_SWFSoundMixerConfig *config,
                                                         size_t budget, FVM_SWFSoundMixer **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!config || config->struct_size != sizeof(*config) ||
        (config->flags & ~(FVM_SWF_SOUND_MIXER_CONVERT | FVM_SWF_SOUND_MIXER_DECLICK)) ||
        !config->channels || config->channels > FVM_SWF_SOUND_MIXER_CHANNELS ||
        !config->voice_capacity || config->voice_capacity > FVM_SWF_SOUND_MIXER_MAX_VOICES)
        return FVM_INVALID_ARGUMENT;
    if (config->flags & FVM_SWF_SOUND_MIXER_CONVERT)
    {
        if (config->sample_rate < FVM_SWF_SOUND_MIXER_MIN_RATE ||
            config->sample_rate > FVM_SWF_SOUND_MIXER_MAX_RATE)
            return FVM_INVALID_ARGUMENT;
    }
    else if (!mixer_rate(config->sample_rate))
        return FVM_INVALID_ARGUMENT;
    if (CHAR_BIT != FVM_SWF_SOUND_BYTE_BITS || sizeof(short) != FVM_SWF_SOUND_UI16_BYTES)
        return FVM_UNSUPPORTED;
    size_t fixed = sizeof(FVM_SWFSoundMixer);
    if (config->voice_capacity > ((size_t)-1 - fixed) / sizeof(FVM_SWFSoundMixerSlot))
        return FVM_OUT_OF_MEMORY;
    fixed += config->voice_capacity * sizeof(FVM_SWFSoundMixerSlot);
    if (fixed > budget)
        return FVM_OUT_OF_MEMORY;
    FVM_SWFSoundMixer *mixer = 0;
    try
    {
        mixer = new FVM_SWFSoundMixer;
        mixer->slots.resize(config->voice_capacity);
        mixer->config = *config;
        mixer->info.sample_rate = config->sample_rate;
        mixer->info.channels = config->channels;
        mixer->info.voice_capacity = config->voice_capacity;
        *out = mixer;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete mixer;
        return FVM_OUT_OF_MEMORY;
    }
}

/* Retain the configured ABI without imposing an additional native-object storage ceiling. */
extern "C" FVM_Result fvm_swf_sound_mixer_create_configured(const FVM_SWFSoundMixerConfig *config,
                                                            FVM_SWFSoundMixer **out)
{
    return fvm_swf_sound_mixer_create_bounded(config, (size_t)-1, out);
}

/* Query the explicit policy without changing phase, voices or caller-owned configuration storage.
 */
extern "C" FVM_Result fvm_swf_sound_mixer_get_config(const FVM_SWFSoundMixer *mixer,
                                                     FVM_SWFSoundMixerConfig *out)
{
    if (!mixer || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    *out = mixer->config;
    return FVM_OK;
}

/* Stop one native sound identity without inspecting any source VM or device queue. */
extern "C" FVM_Result fvm_swf_sound_mixer_stop_id(FVM_SWFSoundMixer *mixer, unsigned int sound_id)
{
    if (!mixer || !sound_id || sound_id > FVM_SWF_SOUND_ID_MAX)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < mixer->slots.size(); ++i)
        if (mixer->slots[i].voice && mixer->slots[i].sound_id == sound_id)
        {
            FVM_SWFSoundMixerSlot &slot = mixer->slots[i];
            if (!(mixer->config.flags & FVM_SWF_SOUND_MIXER_DECLICK))
                retire(mixer, i);
            else if (!slot.release_frames)
            {
                slot.release_frames = mixer->info.sample_rate / FVM_SWF_SOUND_RELEASE_DIVISOR;
                if (slot.release_frames < FVM_SWF_SOUND_RELEASE_MIN)
                    slot.release_frames = FVM_SWF_SOUND_RELEASE_MIN;
                slot.release_left = slot.release_frames;
            }
        }
    return FVM_OK;
}
/* Sum requested retained native storage, including releasing/converted lookahead owners. */
extern "C" FVM_Result fvm_swf_sound_mixer_storage_info(const FVM_SWFSoundMixer *mixer,
                                                        FVM_SWFSoundMixerStorageInfo *out)
{
    if (!mixer || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundMixerStorageInfo info;
    info.struct_size = sizeof(info);
    info.fixed_bytes = sizeof(*mixer) + mixer->slots.size() * sizeof(FVM_SWFSoundMixerSlot);
    info.voice_bytes = 0;
    for (size_t i = 0; i < mixer->slots.size(); ++i)
        if (mixer->slots[i].voice)
        {
            FVM_SWFSoundVoiceStorageInfo voice;
            voice.struct_size = sizeof(voice);
            FVM_Result result = fvm_swf_sound_voice_storage_info(mixer->slots[i].voice, &voice);
            if (result != FVM_OK)
                return result;
            if (voice.retained_bytes > (size_t)-1 - info.voice_bytes)
                return FVM_OUT_OF_MEMORY;
            info.voice_bytes += voice.retained_bytes;
        }
    if (info.voice_bytes > (size_t)-1 - info.fixed_bytes)
        return FVM_OUT_OF_MEMORY;
    info.retained_bytes = info.fixed_bytes + info.voice_bytes;
    *out = info;
    return FVM_OK;
}

/* Match SyncNoMultiple policy without consulting a source context or preparing a voice. */
extern "C" FVM_Result fvm_swf_sound_mixer_sound_active(const FVM_SWFSoundMixer *mixer,
                                                        unsigned int id, int *out)
{
    if (!mixer || !out || !id || id > FVM_SWF_SOUND_ID_MAX)
        return FVM_INVALID_ARGUMENT;
    int active = 0;
    for (size_t i = 0; i < mixer->slots.size(); ++i)
        if (mixer->slots[i].voice && !mixer->slots[i].release_frames &&
            mixer->slots[i].sound_id == id)
        {
            active = 1;
            break;
        }
    *out = active;
    return FVM_OK;
}

/* Adopt prepared independent ownership only after every fallible policy check has succeeded. */
extern "C" FVM_Result fvm_swf_sound_mixer_submit_prepared_bounded(FVM_SWFSoundMixer *mixer,
                                                          FVM_SWFSoundVoice **voice, size_t budget)
{
    if (!mixer || !voice || !*voice)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundVoiceInfo progress;
    progress.struct_size = sizeof(progress);
    FVM_Result result = fvm_swf_sound_voice_info(*voice, &progress);
    if (result != FVM_OK)
        return result;
    if (progress.flags & FVM_SWF_SOUND_EVENT_NO_MULTIPLE)
        for (size_t i = 0; i < mixer->slots.size(); ++i)
            if (mixer->slots[i].voice && !mixer->slots[i].release_frames &&
                mixer->slots[i].sound_id == progress.sound_id)
            {
                fvm_swf_sound_voice_close(*voice);
                *voice = 0;
                return FVM_OK;
            }
    if (!(mixer->config.flags & FVM_SWF_SOUND_MIXER_CONVERT) &&
        (progress.sample_rate != mixer->info.sample_rate || progress.channels != mixer->info.channels))
        return FVM_UNSUPPORTED;
    if (progress.finished)
    {
        fvm_swf_sound_voice_close(*voice);
        *voice = 0;
        return FVM_OK;
    }
    FVM_SWFSoundMixerStorageInfo retained;
    FVM_SWFSoundVoiceStorageInfo incoming;
    retained.struct_size = sizeof(retained);
    incoming.struct_size = sizeof(incoming);
    result = fvm_swf_sound_mixer_storage_info(mixer, &retained);
    if (result == FVM_OK)
        result = fvm_swf_sound_voice_storage_info(*voice, &incoming);
    if (result != FVM_OK)
        return result;
    if (retained.retained_bytes > budget || incoming.retained_bytes > budget - retained.retained_bytes)
        return FVM_OUT_OF_MEMORY;
    for (size_t i = 0; i < mixer->slots.size(); ++i)
    {
        if (!mixer->slots[i].voice)
        {
            FVM_SWFSoundMixerSlot &slot = mixer->slots[i];
            slot.sound_id = progress.sound_id;
            slot.voice = *voice;
            slot.source_rate = progress.sample_rate;
            slot.source_channels = progress.channels;
            slot.phase = 0;
            slot.release_frames = slot.release_left = 0;
            slot.ready = slot.left_valid = slot.right_valid = false;
            slot.converted = progress.sample_rate != mixer->info.sample_rate ||
                             progress.channels != mixer->info.channels;
            ++mixer->info.active_voices;
            *voice = 0;
            return FVM_OK;
        }
    }
    return FVM_OUT_OF_MEMORY;
}

/* Preserve the existing adoption ABI with no additional host-wide storage ceiling. */
extern "C" FVM_Result fvm_swf_sound_mixer_submit_prepared(FVM_SWFSoundMixer *mixer,
                                                          FVM_SWFSoundVoice **voice)
{
    return fvm_swf_sound_mixer_submit_prepared_bounded(mixer, voice, (size_t)-1);
}

/* Resolve control identity before changing slots; a failed snapshot never interrupts old voices. */
extern "C" FVM_Result fvm_swf_sound_mixer_submit(FVM_SWFSoundMixer *mixer, FVM_Context *context,
                                                 size_t ordinal)
{
    if (!mixer)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundEvent event;
    event.struct_size = sizeof(event);
    FVM_Result result = fvm_swf_sound_event_at(context, ordinal, &event);
    if (result != FVM_OK)
        return result;
    /* A stop is meaningful even if its source codec/rate cannot be rendered by this mixer. */
    if (event.flags & FVM_SWF_SOUND_EVENT_STOP)
        return fvm_swf_sound_mixer_stop_id(mixer, event.sound_id);
    if (event.flags & FVM_SWF_SOUND_EVENT_NO_MULTIPLE)
        for (size_t i = 0; i < mixer->slots.size(); ++i)
            if (mixer->slots[i].voice && !mixer->slots[i].release_frames &&
                mixer->slots[i].sound_id == event.sound_id)
                return FVM_OK;
    FVM_SWFSoundInfo source;
    source.struct_size = sizeof(source);
    result = fvm_swf_sound_info(context, event.sound_id, &source);
    if (result != FVM_OK)
        return result;
    if (!(mixer->config.flags & FVM_SWF_SOUND_MIXER_CONVERT) &&
        (source.sample_rate != mixer->info.sample_rate || source.channels != mixer->info.channels))
        return FVM_UNSUPPORTED;
    /* Prepare before publication. Empty/zero-loop events consume no voice slot, even when full. */
    FVM_SWFSoundVoice *voice = 0;
    result = fvm_swf_sound_voice_open(context, ordinal, &voice);
    if (result != FVM_OK)
        return result;
    result = fvm_swf_sound_mixer_submit_prepared(mixer, &voice);
    fvm_swf_sound_voice_close(voice); /* Failure leaves the temporary owner with this caller. */
    return result;
}

/* Pull/map one styled source frame. The owned voice decoder cannot fail with these validated
 * arguments. */
static bool source_frame(FVM_SWFSoundMixerSlot &slot, unsigned int output_channels, long *out)
{
    short raw[FVM_SWF_SOUND_MIXER_CHANNELS];
    size_t received = 0;
    fvm_swf_sound_voice_read(slot.voice, 1, raw, FVM_SWF_SOUND_MIXER_CHANNELS, &received);
    if (!received)
        return false;
    if (output_channels == FVM_SWF_SOUND_MONO_CHANNELS)
    {
        long sum = slot.source_channels == FVM_SWF_SOUND_MONO_CHANNELS
                       ? (long)raw[0] * (long)FVM_SWF_SOUND_MIXER_CHANNELS
                       : (long)raw[0] + raw[1];
        /* Define negative rounding explicitly even on pre-C++11 signed-division implementations. */
        long magnitude = sum < 0 ? -sum : sum;
        long average = magnitude / (long)FVM_SWF_SOUND_MIXER_CHANNELS;
        out[0] = sum < 0 ? -average : average;
    }
    else
    {
        out[0] = raw[0];
        out[1] = slot.source_channels == FVM_SWF_SOUND_MONO_CHANNELS ? raw[0] : raw[1];
    }
    return true;
}

/* Add one converted voice using exact rational phase; hold the final sample for its source
 * interval. */
static bool converted_slice(FVM_SWFSoundMixer *mixer, FVM_SWFSoundMixerSlot &slot, size_t frames)
{
    unsigned int channels = mixer->info.channels;
    unsigned long rate = mixer->info.sample_rate;
    if (!slot.ready)
    {
        slot.left_valid = source_frame(slot, channels, slot.left);
        slot.right_valid = slot.left_valid && source_frame(slot, channels, slot.right);
        slot.ready = true;
    }
    for (size_t frame = 0; frame < frames && slot.left_valid; ++frame)
    {
        for (unsigned int channel = 0; channel < channels; ++channel)
        {
            long left = slot.left[channel];
            long right = slot.right_valid ? slot.right[channel] : left;
            /* Products fit exactly in a double, avoiding non-C++98 64-bit integer extensions.
             * One final division/truncation rounds this voice toward zero before wide summation. */
            double value = ((double)left * (rate - slot.phase) + (double)right * slot.phase) / rate;
            if (slot.release_frames)
                value *= (double)(slot.release_left - 1) / (slot.release_frames - 1);
            mixer->sum[frame * channels + channel] += (long)value;
        }
        if (slot.release_frames && --slot.release_left == 0)
            return false; /* The final release frame is exactly zero in every channel. */
        slot.phase += slot.source_rate;
        while (slot.phase >= rate && slot.left_valid)
        {
            slot.phase -= rate;
            slot.left_valid = slot.right_valid;
            if (slot.left_valid)
            {
                for (unsigned int channel = 0; channel < channels; ++channel)
                    slot.left[channel] = slot.right[channel];
                slot.right_valid = source_frame(slot, channels, slot.right);
            }
        }
    }
    return slot.left_valid;
}

/* Mix in bounded passes: wide addition precedes one final saturation, independent of voice order.
 */
static FVM_Result mixer_read(FVM_SWFSoundMixer *mixer, size_t frames, short *out,
                              size_t capacity, size_t *written, bool additive)
{
    if (!written)
        return FVM_INVALID_ARGUMENT;
    *written = 0;
    if (!mixer || frames > capacity / mixer->info.channels || (frames && !out))
        return FVM_INVALID_ARGUMENT;
    size_t at = 0;
    while (at < frames)
    {
        size_t count = frames - at;
        if (count > FVM_SWF_SOUND_MIXER_CHUNK)
            count = FVM_SWF_SOUND_MIXER_CHUNK;
        size_t values = count * mixer->info.channels;
        for (size_t j = 0; j < values; ++j)
            mixer->sum[j] = additive ? out[at * mixer->info.channels + j] : 0;
        for (size_t i = 0; i < mixer->slots.size(); ++i)
        {
            FVM_SWFSoundVoice *voice = mixer->slots[i].voice;
            if (!voice)
                continue;
            if (mixer->slots[i].converted)
            {
                if (!converted_slice(mixer, mixer->slots[i], count))
                    retire(mixer, i);
                continue;
            }
            size_t received = 0;
            /* Owned, validated geometry makes read infallible here; short tails are silence. */
            fvm_swf_sound_voice_read(voice, count, mixer->source, values, &received);
            FVM_SWFSoundMixerSlot &slot = mixer->slots[i];
            bool released = false;
            for (size_t frame = 0; frame < received / mixer->info.channels; ++frame)
            {
                double gain = slot.release_frames
                                  ? (double)(slot.release_left - 1) / (slot.release_frames - 1)
                                  : 1.0;
                for (unsigned int channel = 0; channel < mixer->info.channels; ++channel)
                {
                    size_t j = frame * mixer->info.channels + channel;
                    mixer->sum[j] += (long)(mixer->source[j] * gain);
                }
                if (slot.release_frames && --slot.release_left == 0)
                {
                    released = true;
                    break;
                }
            }
            FVM_SWFSoundVoiceInfo progress;
            progress.struct_size = sizeof(progress);
            fvm_swf_sound_voice_info(voice, &progress);
            if (progress.finished || released)
                retire(mixer, i);
        }
        for (size_t j = 0; j < values; ++j)
        {
            long value = mixer->sum[j];
            if (value < FVM_SWF_SOUND_SHORT_MIN)
                value = FVM_SWF_SOUND_SHORT_MIN;
            if (value > FVM_SWF_SOUND_SHORT_MAX)
                value = FVM_SWF_SOUND_SHORT_MAX;
            out[at * mixer->info.channels + j] = (short)value;
        }
        at += count;
    }
    *written = frames * mixer->info.channels;
    return FVM_OK;
}

/* Preserve legacy zero-base output, including silence after the last prepared voice. */
extern "C" FVM_Result fvm_swf_sound_mixer_read(FVM_SWFSoundMixer *mixer, size_t frames, short *out,
                                               size_t capacity, size_t *written)
{
    return mixer_read(mixer, frames, out, capacity, written, false);
}

/* Add all native event voices to caller PCM in wide precision before the only clipping pass. */
extern "C" FVM_Result fvm_swf_sound_mixer_add(FVM_SWFSoundMixer *mixer, size_t frames, short *pcm,
                                              size_t capacity, size_t *written)
{
    return mixer_read(mixer, frames, pcm, capacity, written, true);
}

/* Clamp source repeats before multiplication; large authored durations never lose low bits. */
static size_t source_horizon(const FVM_SWFSoundVoiceInfo &info, size_t bound)
{
    if (info.finished) return 0;
    size_t remaining = info.frames_per_play - info.frame_in_play;
    if (remaining >= bound) return bound;
    unsigned int repeats = info.play_count - info.play_index - 1;
    if (repeats)
    {
        size_t room = bound - remaining;
        if (info.frames_per_play > room / repeats) return bound;
        remaining += info.frames_per_play * repeats;
    }
    return remaining;
}

/* Observe bounded remaining output on the live conversion grid without consuming lookahead. */
extern "C" FVM_Result fvm_swf_sound_mixer_active_frames(const FVM_SWFSoundMixer *mixer,
                                                         size_t maximum, size_t *out)
{
    if (!mixer || !out || !maximum || maximum > FVM_SWF_SOUND_MIXER_MAX_HORIZON)
        return FVM_INVALID_ARGUMENT;
    size_t longest = 0;
    for (size_t i = 0; i < mixer->slots.size(); ++i)
    {
        const FVM_SWFSoundMixerSlot &slot = mixer->slots[i];
        if (!slot.voice) continue;
        FVM_SWFSoundVoiceInfo info;
        info.struct_size = sizeof(info);
        FVM_Result result = fvm_swf_sound_voice_info(slot.voice, &info);
        if (result != FVM_OK) return result;
        size_t count = 0;
        if (!slot.converted)
            count = source_horizon(info, maximum);
        else
        {
            /* Products stay exact below 2^53 under the public rate/horizon bounds. */
            size_t bound = (size_t)((double)maximum * slot.source_rate / mixer->info.sample_rate)
                           + FVM_SWF_SOUND_MIXER_HORIZON_GUARD;
            size_t source = source_horizon(info, bound);
            if (slot.ready)
            {
                if (slot.left_valid && source < bound) ++source;
                if (slot.right_valid && source < bound) ++source;
            }
            double numerator = (double)source * mixer->info.sample_rate - slot.phase;
            if (numerator > 0)
            {
                double frames = numerator / slot.source_rate;
                count = frames >= maximum ? maximum : (size_t)frames;
                if (count < maximum && (double)count * slot.source_rate < numerator) ++count;
            }
        }
        if (slot.release_frames && count > slot.release_left) count = slot.release_left;
        if (count > longest) longest = count;
        if (longest == maximum) break;
    }
    *out = longest;
    return FVM_OK;
}

/* Return copied activity without consulting a source context or a hardware clock. */
extern "C" FVM_Result fvm_swf_sound_mixer_info(const FVM_SWFSoundMixer *mixer,
                                               FVM_SWFSoundMixerInfo *out)
{
    if (!mixer || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    *out = mixer->info;
    return FVM_OK;
}

/* Explicitly discard current software voices; a separate host operation must flush device queues.
 */
extern "C" FVM_Result fvm_swf_sound_mixer_stop_all(FVM_SWFSoundMixer *mixer)
{
    if (!mixer)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < mixer->slots.size(); ++i)
        retire(mixer, i);
    return FVM_OK;
}

/* [GC:] Finish native ownership; no callbacks, allocation or context access occur. */
extern "C" void fvm_swf_sound_mixer_close(FVM_SWFSoundMixer *mixer)
{
    delete mixer;
}
