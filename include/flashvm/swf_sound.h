/* FlashVM: bounded views and explicit PCM submission for loaded SWF DefineSound resources. */
#ifndef FLASHVM_SWF_SOUND_H
#define FLASHVM_SWF_SOUND_H /* Guard the C89 sound-resource API. */
#include "multimedia.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define FVM_SWF_SOUND_PCM_NATIVE 0U /* Legacy uncompressed PCM follows the playback host byte order. */
#define FVM_SWF_SOUND_ADPCM 1U /* SWF differential PCM with two through five bits per code. */
#define FVM_SWF_SOUND_PCM_LE 3U /* SWF4+ uncompressed little-endian PCM. */
    /* Borrowed nominal metadata and complete SoundData for one root dictionary definition. */
    typedef struct FVM_SWFSoundInfo
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFSoundInfo), preserved on successful publication. */
        unsigned int sound_id; /* Root-dictionary UI16 identifier, not a display-list depth. */
        unsigned int format; /* Raw SoundFormat nibble; unsupported codecs remain opaque. */
        unsigned int sample_rate; /* Nominal header rate: 5512, 11025, 22050 or 44100 Hz. */
        unsigned int bits_per_sample; /* Header 8/16-bit flag; compressed codecs can ignore it. */
        unsigned int channels; /* Header mono/stereo count; compressed codecs can override it. */
        unsigned long sample_count; /* Declared sample frames, including both channels of each stereo pair. */
        const unsigned char
            *data; /* Borrowed complete SoundData bytes, valid until successful reload or context destruction. */
        size_t data_size; /* Exact SoundData byte count; codec-specific prefixes are retained. */
    } FVM_SWFSoundInfo;
#define FVM_SWF_SOUND_ENVELOPE_UNITY 32768U /* Maximum envelope gain, representing unity amplitude. */
#define FVM_SWF_SOUND_CONTROL_RATE 44100UL /* InPoint/OutPoint positions use this fixed control clock in Hz. */
#define FVM_SWF_SOUND_ENVELOPE_RATE 44100UL /* Envelope positions use this fixed sample clock in Hz. */
#define FVM_SWF_SOUND_EVENT_IN_POINT 1U /* SOUNDINFO contains an explicit input sample position. */
#define FVM_SWF_SOUND_EVENT_OUT_POINT 2U /* SOUNDINFO contains an explicit last sample position. */
#define FVM_SWF_SOUND_EVENT_LOOPS 4U /* SOUNDINFO contains an explicit loop count. */
#define FVM_SWF_SOUND_EVENT_ENVELOPE 8U /* SOUNDINFO contains an explicit envelope array, possibly empty. */
#define FVM_SWF_SOUND_EVENT_NO_MULTIPLE 16U /* Request starting only if this sound is not already playing. */
#define FVM_SWF_SOUND_EVENT_STOP 32U /* Request stopping this sound rather than starting it. */
    /* Copied root StartSound control metadata, without scheduling or borrowing storage. */
    typedef struct FVM_SWFSoundEvent
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFSoundEvent). */
        unsigned int sound_id; /* Unique root DefineSound identifier, resolved by event_at. */
        unsigned int frame; /* One-based root timeline frame containing this control. */
        unsigned int flags; /* SOUNDINFO presence and synchronization bits, preserved verbatim. */
        unsigned long in_point; /* Raw 44100 Hz start position; zero when absent, presence distinguished by flags. */
        unsigned long out_point; /* Raw 44100 Hz last-sample position; zero when absent, not an exclusive bound. */
        unsigned int loop_count; /* Raw UI16 count; zero when absent, without playback normalization. */
        unsigned int envelope_count; /* Raw UI8 number of envelope points, zero when absent. */
    } FVM_SWFSoundEvent;
    /* Copied envelope point with its own 44.1 kHz time base. */
    typedef struct FVM_SWFSoundEnvelope
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFSoundEnvelope). */
        unsigned long position_44; /* Position measured in 44100 Hz sample units, independent of sound rate. */
        unsigned int left_level; /* Left gain, 0..32768 inclusive. */
        unsigned int right_level; /* Right gain, 0..32768 inclusive; mono playback averages both gains. */
    } FVM_SWFSoundEnvelope;
    /* Count syntactically valid root StartSound occurrences; no resource resolution or playback. */
    FVM_API FVM_Result fvm_swf_sound_event_count(FVM_Context *, size_t *);
    /* Copy a zero-based StartSound occurrence after resolving its unique sound definition. */
    FVM_API FVM_Result fvm_swf_sound_event_at(FVM_Context *, size_t, FVM_SWFSoundEvent *);
    /* Re-resolve a root event and copy one zero-based envelope point; no borrowed pointers escape. */
    FVM_API FVM_Result fvm_swf_sound_event_envelope(FVM_Context *, size_t, size_t, FVM_SWFSoundEnvelope *);
    /* Count validated root DefineSound occurrences, without allocating or opening output. */
    FVM_API FVM_Result fvm_swf_sound_count(FVM_Context *, size_t *);
    /* Inspect a zero-based root occurrence; reject an ambiguous duplicate selected sound ID. */
    FVM_API FVM_Result fvm_swf_sound_at(FVM_Context *, size_t, FVM_SWFSoundInfo *);
    /* Inspect a unique root DefineSound in canonical loaded bytes; no allocations or callbacks. */
    FVM_API FVM_Result fvm_swf_sound_info(FVM_Context *, unsigned int, FVM_SWFSoundInfo *);
    /* Decode an explicit frame range of native/little-endian 8/16-bit PCM or SWF ADPCM into caller shorts.
     * capacity/written count channel values, not frames; failure writes no samples. */
    FVM_API FVM_Result fvm_swf_sound_read_pcm(FVM_Context *, unsigned int, unsigned long, size_t, short *, size_t,
                                              size_t *);
    /* Apply one root StartSound to a dedicated single-voice channel; render bounded styles before output.
     * Stops/no-multiple affect all bound same-ID voices in this context; an ordinary start requires an inactive channel. */
    FVM_API FVM_Result fvm_swf_sound_event_submit(FVM_Context *, size_t, FVM_AudioChannel *);
    /* Copy/decode one bounded range and synchronously submit to a channel owned by this context.
     * Does not execute StartSound, schedule clocks, loop, mix or apply envelopes. */
    FVM_API FVM_Result fvm_swf_sound_submit(FVM_Context *, unsigned int, FVM_AudioChannel *, unsigned long, size_t);
#ifdef __cplusplus
}
#endif
#endif
