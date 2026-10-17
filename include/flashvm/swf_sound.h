/* FlashVM: bounded views and explicit PCM submission for loaded SWF DefineSound resources. */
#ifndef FLASHVM_SWF_SOUND_H
#define FLASHVM_SWF_SOUND_H /* Guard the C89 sound-resource API. */
#include "multimedia.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define FVM_SWF_SOUND_PCM_NATIVE 0U /* Legacy uncompressed PCM follows the playback host byte order. */
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
    /* Count validated root DefineSound occurrences, without allocating or opening output. */
    FVM_API FVM_Result fvm_swf_sound_count(FVM_Context *, size_t *);
    /* Inspect a zero-based root occurrence; reject an ambiguous duplicate selected sound ID. */
    FVM_API FVM_Result fvm_swf_sound_at(FVM_Context *, size_t, FVM_SWFSoundInfo *);
    /* Inspect a unique root DefineSound in canonical loaded bytes; no allocations or callbacks. */
    FVM_API FVM_Result fvm_swf_sound_info(FVM_Context *, unsigned int, FVM_SWFSoundInfo *);
    /* Decode an explicit frame range of raw/native or little-endian 16-bit PCM into caller shorts.
     * capacity/written count channel values, not frames; failure writes no samples. */
    FVM_API FVM_Result fvm_swf_sound_read_pcm(FVM_Context *, unsigned int, unsigned long, size_t, short *, size_t,
                                              size_t *);
    /* Copy/decode one bounded range and synchronously submit to a channel owned by this context.
     * Does not execute StartSound, schedule clocks, loop, mix or apply envelopes. */
    FVM_API FVM_Result fvm_swf_sound_submit(FVM_Context *, unsigned int, FVM_AudioChannel *, unsigned long, size_t);
#ifdef __cplusplus
}
#endif
#endif
