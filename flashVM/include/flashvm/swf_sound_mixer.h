/* FlashVM: explicit, bounded StartSound voice mixing without an implicit device clock. */
#ifndef FLASHVM_SWF_SOUND_MIXER_H
#define FLASHVM_SWF_SOUND_MIXER_H /* Guard the independent C89 mixer ABI. */
#include "swf_sound.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define FVM_SWF_SOUND_MIXER_MAX_VOICES                                                             \
    64U /* Bound signed-long accumulation on every supported host. */
    typedef struct FVM_SWFSoundMixer
        FVM_SWFSoundMixer; /* Native owner; independent of source VM lifetime. */
    typedef struct FVM_SWFSoundMixerInfo
    {
        unsigned long struct_size;   /* Required sizeof(FVM_SWFSoundMixerInfo). */
        unsigned int sample_rate;    /* Fixed native PCM clock in Hz; no resampling is implicit. */
        unsigned int channels;       /* Fixed interleaved channel count, one or two. */
        unsigned int voice_capacity; /* Maximum simultaneous voices, chosen on create. */
        unsigned int active_voices;  /* Current nonempty/nonexhausted voice count. */
    } FVM_SWFSoundMixerInfo;
#define FVM_SWF_SOUND_MIXER_CONVERT                                                                \
    1UL /* Explicit linear rate/channel conversion; zero keeps strict geometry. */
#define FVM_SWF_SOUND_MIXER_DECLICK                                                                \
    2UL /* Opt-in five-millisecond release for stop-by-resource events. */
#define FVM_SWF_SOUND_MIXER_MIN_RATE 5512U /* Lowest configured output clock in integer Hz. */
#define FVM_SWF_SOUND_MIXER_MAX_RATE                                                               \
    192000U /* Highest configured output clock; bounds rational phase arithmetic. */
    typedef struct FVM_SWFSoundMixerConfig
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFSoundMixerConfig). */
        unsigned int sample_rate;  /* Output Hz; strict mode requires a SWF nominal rate. */
        unsigned int channels; /* Output mono/stereo geometry, not a source-format restriction in
                                  convert mode. */
        unsigned int voice_capacity; /* Maximum simultaneous nonempty voices, 1..MAX_VOICES. */
        unsigned long flags;         /* CONVERT and/or DECLICK; zero retains exact legacy policy. */
    } FVM_SWFSoundMixerConfig;
    /* Create with explicit conversion policy; clear *out on all failures, copy caller config on
     * success. */
    FVM_API FVM_Result fvm_swf_sound_mixer_create_configured(const FVM_SWFSoundMixerConfig *,
                                                             FVM_SWFSoundMixer **out);
    /* Copy the immutable configuration; wrong size/null arguments preserve output. */
    FVM_API FVM_Result fvm_swf_sound_mixer_get_config(const FVM_SWFSoundMixer *,
                                                      FVM_SWFSoundMixerConfig *);
    /* Allocate a fixed-capacity mixer for a SWF nominal rate and mono/stereo; clear *out first. */
    FVM_API FVM_Result fvm_swf_sound_mixer_create(unsigned int sample_rate, unsigned int channels,
                                                  unsigned int voice_capacity,
                                                  FVM_SWFSoundMixer **out);
    /* Apply one explicit root event: stop/no-multiple by sound ID; otherwise snapshot a new voice.
     * Errors preserve existing voices. A full mixer returns OUT_OF_MEMORY, never steals a voice. */
    FVM_API FVM_Result fvm_swf_sound_mixer_submit(FVM_SWFSoundMixer *, FVM_Context *,
                                                  size_t event_index);
    /* Fill exactly frames with native S16, including silence; capacity/written count channel
     * values. Validate before advancing; mix in wide precision and saturate once. No
     * allocation/callback. */
    FVM_API FVM_Result fvm_swf_sound_mixer_read(FVM_SWFSoundMixer *, size_t frames,
                                                short *destination, size_t capacity_values,
                                                size_t *written_values);
    /* Copy fixed geometry and current activity, preserving output on invalid input. */
    FVM_API FVM_Result fvm_swf_sound_mixer_info(const FVM_SWFSoundMixer *, FVM_SWFSoundMixerInfo *);
    /* Discard every voice immediately; no device stop or context callback is entered. */
    FVM_API FVM_Result fvm_swf_sound_mixer_stop_all(FVM_SWFSoundMixer *);
    /* Release voices and mixer; null is accepted. All operations require external serialization. */
    FVM_API void fvm_swf_sound_mixer_close(FVM_SWFSoundMixer *);
#ifdef __cplusplus
}
#endif
#endif
