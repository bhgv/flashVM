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
#define FVM_SWF_SOUND_MP3 2U /* SWF4+ MPEG Layer III with a signed initial-seek prefix. */
#define FVM_SWF_SOUND_PCM_LE 3U /* SWF4+ uncompressed little-endian PCM. */
    /* Borrowed nominal metadata and complete SoundData for one root dictionary definition. */
    typedef struct FVM_SWFSoundInfo
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFSoundInfo), preserved on successful publication. */
        unsigned int sound_id; /* Root-dictionary UI16 identifier, not a display-list depth. */
        unsigned int format; /* Raw SoundFormat nibble; unsupported codecs remain opaque. */
        unsigned int sample_rate; /* Nominal PCM rate; supported MP3 uses effective first-packet MPEG rate. */
        unsigned int bits_per_sample; /* Header 8/16-bit flag; compressed codecs can ignore it. */
        unsigned int channels; /* Header mono/stereo count; supported MP3 uses effective MPEG channels. */
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
    /* Read-only sound/event metadata queries share native execution work and worker stops.
     * Exhaustion returns ERROR with unchanged caller outputs; see SOUND_SNAPSHOT.md.
     * These queries allocate nothing and run on the context owner thread. */
    /* Count syntactically valid root StartSound occurrences; no resource resolution or playback. */
    FVM_API FVM_Result fvm_swf_sound_event_count(FVM_Context *, size_t *);
    /* Copy a zero-based StartSound occurrence after resolving its unique sound definition. */
    FVM_API FVM_Result fvm_swf_sound_event_at(FVM_Context *, size_t, FVM_SWFSoundEvent *);
    /* Re-resolve a root event and copy one zero-based envelope point; no borrowed pointers escape. */
    FVM_API FVM_Result fvm_swf_sound_event_envelope(FVM_Context *, size_t, size_t, FVM_SWFSoundEnvelope *);
    typedef struct FVM_SWFSoundVoice FVM_SWFSoundVoice; /* Host-owned native snapshot; close independently of VM lifetime. */
    /* Copied immutable geometry and current incremental position for one prepared event voice. */
    typedef struct FVM_SWFSoundVoiceInfo
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFSoundVoiceInfo). */
        unsigned int sound_id; /* Copied root resource identity; not a live VM reference. */
        unsigned int flags; /* Copied SOUNDINFO flags; synchronization is not executed by snapshot reads. */
        unsigned int sample_rate; /* Native PCM rate retained from the resource header. */
        unsigned int channels; /* Native interleaved mono/stereo channel count. */
        unsigned long source_first; /* First decoded source frame after fixed-clock range conversion. */
        size_t frames_per_play; /* Frames in one selected source interval; repetitions do not expand storage. */
        unsigned int play_count; /* Total requested plays, including explicit zero. */
        unsigned int play_index; /* Current zero-based play; equals play_count after nonempty exhaustion. */
        size_t frame_in_play; /* Next source-relative frame in the current play; zero after final exhaustion. */
        int finished; /* Nonzero for zero plays, empty intervals or exhausted repetitions. */
    } FVM_SWFSoundVoiceInfo;
    /* Snapshot one root event's decoded interval and envelope; publish null on failure, reject stop controls. */
    FVM_API FVM_Result fvm_swf_sound_voice_open(FVM_Context *, size_t, FVM_SWFSoundVoice **);
    /* Requested native storage, excluding allocator overhead, VM, decoder stack and host queues. */
    typedef struct FVM_SWFSoundVoiceStorageInfo
    {
        size_t struct_size; /* Caller supplies exact sizeof this observation. */
        size_t retained_bytes; /* Native object plus PCM and envelope arrays; no hidden capacity. */
        size_t pcm_bytes; /* Selected interval only; repetitions do not multiply storage. */
        size_t envelope_bytes; /* Copied ordered envelope records, including ABI padding. */
    } FVM_SWFSoundVoiceStorageInfo;
    /* Same source validation/runtime payload limit as voice_open, plus an explicit object/PCM/
     * envelope ceiling. Zero is valid and returns OUT_OF_MEMORY even for empty voices. Clear
     * *out before validation; never allocate the voice or its arrays above the ceiling. May
     * decode/allocate on the context owner; no callback, VM execution or device access. Result
     * owns its copies independently of context/GC/reload. Other errors follow voice_open.
     * Serialize the source context; budget is requested storage, not process RSS or CPU work. */
    FVM_API FVM_Result fvm_swf_sound_voice_open_bounded(FVM_Context *, size_t event_index,
                                                        size_t budget_bytes, FVM_SWFSoundVoice **out);
    /* No allocation/callback/VM; null or wrong struct_size INVALID_ARGUMENT, output preserved.
     * Retain and externally serialize the native voice until return. */
    FVM_API FVM_Result fvm_swf_sound_voice_storage_info(const FVM_SWFSoundVoice *,
                                                        FVM_SWFSoundVoiceStorageInfo *);
    /* Copy geometry/progress without allocations, callbacks or VM access. */
    FVM_API FVM_Result fvm_swf_sound_voice_info(const FVM_SWFSoundVoice *, FVM_SWFSoundVoiceInfo *);
    /* Read at most requested frames as native S16, advancing only after argument validation; EOF is NOT_FOUND. */
    FVM_API FVM_Result fvm_swf_sound_voice_read(FVM_SWFSoundVoice *, size_t, short *, size_t, size_t *);
    /* Release native snapshot storage; null is accepted, no VM/host output callback is entered. */
    FVM_API FVM_Result fvm_swf_sound_voice_close(FVM_SWFSoundVoice *);
    /* Count validated root DefineSound occurrences, without allocating or opening output. */
    FVM_API FVM_Result fvm_swf_sound_count(FVM_Context *, size_t *);
    /* Inspect a zero-based root occurrence; reject an ambiguous duplicate selected sound ID. */
    FVM_API FVM_Result fvm_swf_sound_at(FVM_Context *, size_t, FVM_SWFSoundInfo *);
    /* Inspect a unique root DefineSound in canonical loaded bytes; no allocations or callbacks. */
    FVM_API FVM_Result fvm_swf_sound_info(FVM_Context *, unsigned int, FVM_SWFSoundInfo *);
    /* Decode an explicit native/little-endian PCM, ADPCM or MP3 frame range into caller S16.
     * MP3 requires SWF4+, standard-bitrate Layer III, stable effective rate/channels and a
     * nonnegative initial SeekSamples prefix. Skip that latency once; require at least the
     * complete declared sample_count after it. Validate every complete MPEG packet, even for
     * zero/early reads; trailing junk/truncation, changed geometry or missing reservoir is
     * INVALID_ARGUMENT. Negative latency/free-format/other layers UNSUPPORTED.
     * MP3 metadata reports effective first-packet geometry when a valid packet is available;
     * malformed/unknown metadata can remain opaque until decode. No implicit gapless tags or
     * extra encoder-delay correction. Range is relative to latency-trimmed declared frames.
     * capacity/written count channel values, not frames; written cleared first, no PCM change
     * on failure. No allocation/callback/VM execution. Whole-source validation is repeated on
     * every MP3 read; prepare a voice once for repeated playback. Retain/serialize source and
     * caller buffers until return; input/output/count storage must be disjoint. */
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
