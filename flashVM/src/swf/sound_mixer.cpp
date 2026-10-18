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
    /* Make construction/partial allocation cleanup safe before accepting any events. */
    FVM_SWFSoundMixerSlot() : voice(0), sound_id(0)
    {
    }
};

/* [GC:] Store native owners only: no context pointer, managed root or borrowed SWF data. */
struct FVM_SWFSoundMixer
{
    FVM_SWFSoundMixerInfo info; /* Fixed geometry and exact occupied slot count. */
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

/* Create the bounded control/output owner before any source VM or output device is needed. */
extern "C" FVM_Result fvm_swf_sound_mixer_create(unsigned int rate, unsigned int channels,
                                                 unsigned int capacity, FVM_SWFSoundMixer **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!mixer_rate(rate) || !channels || channels > FVM_SWF_SOUND_MIXER_CHANNELS || !capacity ||
        capacity > FVM_SWF_SOUND_MIXER_MAX_VOICES)
        return FVM_INVALID_ARGUMENT;
    if (CHAR_BIT != FVM_SWF_SOUND_BYTE_BITS || sizeof(short) != FVM_SWF_SOUND_UI16_BYTES)
        return FVM_UNSUPPORTED;
    FVM_SWFSoundMixer *mixer = 0;
    try
    {
        mixer = new FVM_SWFSoundMixer;
        mixer->slots.resize(capacity);
        mixer->info.sample_rate = rate;
        mixer->info.channels = channels;
        mixer->info.voice_capacity = capacity;
        *out = mixer;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete mixer;
        return FVM_OUT_OF_MEMORY;
    }
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
    {
        for (size_t i = 0; i < mixer->slots.size(); ++i)
            if (mixer->slots[i].sound_id == event.sound_id)
                retire(mixer, i);
        return FVM_OK;
    }
    if (event.flags & FVM_SWF_SOUND_EVENT_NO_MULTIPLE)
        for (size_t i = 0; i < mixer->slots.size(); ++i)
            if (mixer->slots[i].voice && mixer->slots[i].sound_id == event.sound_id)
                return FVM_OK;
    FVM_SWFSoundInfo source;
    source.struct_size = sizeof(source);
    result = fvm_swf_sound_info(context, event.sound_id, &source);
    if (result != FVM_OK)
        return result;
    if (source.sample_rate != mixer->info.sample_rate || source.channels != mixer->info.channels)
        return FVM_UNSUPPORTED;
    /* Prepare before publication. Empty/zero-loop events consume no voice slot, even when full. */
    FVM_SWFSoundVoice *voice = 0;
    result = fvm_swf_sound_voice_open(context, ordinal, &voice);
    if (result != FVM_OK)
        return result;
    FVM_SWFSoundVoiceInfo progress;
    progress.struct_size = sizeof(progress);
    fvm_swf_sound_voice_info(voice, &progress);
    if (progress.finished)
    {
        fvm_swf_sound_voice_close(voice);
        return FVM_OK;
    }
    for (size_t i = 0; i < mixer->slots.size(); ++i)
    {
        if (!mixer->slots[i].voice)
        {
            mixer->slots[i].sound_id = event.sound_id;
            mixer->slots[i].voice = voice;
            ++mixer->info.active_voices;
            return FVM_OK;
        }
    }
    fvm_swf_sound_voice_close(voice);
    return FVM_OUT_OF_MEMORY;
}

/* Mix in bounded passes: wide addition precedes one final saturation, independent of voice order.
 */
extern "C" FVM_Result fvm_swf_sound_mixer_read(FVM_SWFSoundMixer *mixer, size_t frames, short *out,
                                               size_t capacity, size_t *written)
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
            mixer->sum[j] = 0;
        for (size_t i = 0; i < mixer->slots.size(); ++i)
        {
            FVM_SWFSoundVoice *voice = mixer->slots[i].voice;
            if (!voice)
                continue;
            size_t received = 0;
            /* Owned, validated geometry makes read infallible here; short tails are silence. */
            fvm_swf_sound_voice_read(voice, count, mixer->source, values, &received);
            for (size_t j = 0; j < received; ++j)
                mixer->sum[j] += mixer->source[j];
            FVM_SWFSoundVoiceInfo progress;
            progress.struct_size = sizeof(progress);
            fvm_swf_sound_voice_info(voice, &progress);
            if (progress.finished)
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
