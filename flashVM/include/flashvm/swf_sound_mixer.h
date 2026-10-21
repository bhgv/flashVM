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
    /* Transfer one prepared native voice at its CURRENT progress into this mixer, no allocation.
     * voice and *voice are required live disjoint ownership storage; retain and serialize all
     * owners until return. Success consumes *voice and clears it, including finished/NoMultiple
     * no-ops (those close the unused voice). Failure preserves handle, progress and old slots.
     * Strict geometry mismatch UNSUPPORTED; full capacity OUT_OF_MEMORY; bad pointers
     * INVALID_ARGUMENT. NoMultiple sees only active non-releasing same-ID voices and can
     * succeed without rendering a mismatched source. Conversion starts a new phase at the
     * current voice cursor; never rewinds it. No VM, callback, device or managed/GC pointer.
     * Caller must not read/close an adopted handle. Externally serialize mixer/voice/queue:
     * adoption affects FUTURE composition, never an already mixed/pending/accepted PCM block. */
    FVM_API FVM_Result fvm_swf_sound_mixer_submit_prepared(FVM_SWFSoundMixer *,
                                                          FVM_SWFSoundVoice **voice);
    /* Same transfer/control policy as submit_prepared, with an aggregate retained-byte ceiling
     * for mixer object, fixed slots and all old plus incoming voice objects/PCM/envelopes.
     * Checked subtraction prevents overflow. Budget excess OUT_OF_MEMORY preserves incoming
     * ownership/progress and old slots. Successful NoMultiple/finished consumption needs no new
     * retained storage and ignores the ceiling. No allocation/VM/callback/device/GC; serialize
     * all borrowed owners. This does not account for host template copies or output queues. */
    FVM_API FVM_Result fvm_swf_sound_mixer_submit_prepared_bounded(FVM_SWFSoundMixer *,
                                                          FVM_SWFSoundVoice **voice, size_t budget_bytes);
    /* Requested native storage only; excludes allocator overhead, queues, VM and host templates. */
    typedef struct FVM_SWFSoundMixerStorageInfo
    {
        size_t struct_size; /* Caller supplies exact sizeof this observation. */
        size_t fixed_bytes; /* Mixer object/scratch and exact preallocated slot table. */
        size_t voice_bytes; /* All owned voice objects, PCM and envelopes, including release tails. */
        size_t retained_bytes; /* Checked sum of fixed_bytes and voice_bytes. */
    } FVM_SWFSoundMixerStorageInfo;
    /* Allocation/callback/VM-free copied observation; null/wrong size INVALID_ARGUMENT, checked
     * size overflow OUT_OF_MEMORY; preserve output on error. Retain/serialize mixer until return. */
    FVM_API FVM_Result fvm_swf_sound_mixer_storage_info(const FVM_SWFSoundMixer *,
                                                        FVM_SWFSoundMixerStorageInfo *);
    /* Query non-releasing same-ID activity, exactly the SyncNoMultiple admission condition.
     * ID 1..65535, output required; invalid input INVALID_ARGUMENT with unchanged output.
     * No allocation/callback/VM/GC/device. Retain/serialize mixer; this is an observation, not a
     * reservation. A later adoption must recheck policy under the same external lock. */
    FVM_API FVM_Result fvm_swf_sound_mixer_sound_active(const FVM_SWFSoundMixer *,
                                                        unsigned int sound_id, int *out);
    /* Stop a dictionary identity 1..65535 using this mixer's existing immediate/DECLICK policy.
     * Null mixer/bad ID INVALID_ARGUMENT before mutation; missing ID OK no-op. No allocation,
     * VM/callback/device/GC or new ownership. Requires serialized live mixer. Does not clear
     * queued/pending PCM, rewind voices or cancel an external producer; future composition only. */
    FVM_API FVM_Result fvm_swf_sound_mixer_stop_id(FVM_SWFSoundMixer *, unsigned int sound_id);
    /* Fill exactly frames with native S16, including silence; capacity/written count channel
     * values. Validate before advancing; mix in wide precision and saturate once. No
     * allocation/callback. */
    FVM_API FVM_Result fvm_swf_sound_mixer_read(FVM_SWFSoundMixer *, size_t frames,
                                                short *destination, size_t capacity_values,
                                                size_t *written_values);
    /* Add current event voices to initialized caller PCM in place, at the configured geometry.
     * Accumulate base plus every voice in wide precision, saturate once; no allocation/callback/VM.
     * frames count interleaved frames; capacity/written count scalar S16 values. Clear *written
     * first. Null mixer/bad storage/capacity INVALID_ARGUMENT without touching PCM/voice progress;
     * null written INVALID_ARGUMENT. Zero frames permits null PCM, advances nothing. No voices
     * leaves base PCM unchanged. Success consumes voice time exactly once, including zero base.
     * Caller owns initialized aligned PCM; keep it disjoint from written and private mixer state.
     * Serialize mixer and PCM externally. For backend retry retain the already mixed bytes;
     * calling add again would advance voices and mix twice. Tails require explicit zero base. */
    FVM_API FVM_Result fvm_swf_sound_mixer_add(FVM_SWFSoundMixer *, size_t frames, short *pcm,
                                               size_t capacity_values, size_t *written_values);
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
