/* FlashVM: explicit serialized native PCM queue pumping and dequeue-clock observation. */
#ifndef FLASHVM_SWF_SOUND_OUTPUT_H
#define FLASHVM_SWF_SOUND_OUTPUT_H /* Guard the independent C89 queue/output ABI. */
#include "swf_sound_mixer.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Backend table version and independent resource/accounting bounds. */
#define FVM_AUDIO_QUEUE_ABI_VERSION 1UL         /* Exact native callback-table ABI. */
#define FVM_SWF_SOUND_OUTPUT_MAX_BLOCK 65536UL  /* Frames in one preparation/retry buffer. */
#define FVM_SWF_SOUND_OUTPUT_MAX_QUEUE 262144UL /* Maximum requested FIFO frames. */
#define FVM_SWF_SOUND_OUTPUT_CLOCK_BITS 53      /* Required double mantissa precision. */
#define FVM_SWF_SOUND_OUTPUT_MAX_CLOCK 9007199254740991.0 /* Exact frame accounting limit. */
    /* Copied native callback table; the host retains ownership of userdata/device. */
    typedef struct FVM_AudioQueueBackend
    {
        unsigned long struct_size; /* Required sizeof(FVM_AudioQueueBackend). */
        unsigned long abi_version; /* Required FVM_AUDIO_QUEUE_ABI_VERSION. */
        unsigned int sample_rate;  /* Actual queue input clock, in frames per second. */
        unsigned int channels;     /* Actual queue input channels, native interleaved S16. */
        void *userdata; /* Borrowed until output close; backend/device remains host-owned. */
        /* Copy all frames before OK; accept none on any error/exception. */
        FVM_Result (*write)(void *userdata, const short *pcm, size_t frames);
        /* Report complete FIFO frames, including while paused; preserve out on error. */
        FVM_Result (*queued)(void *userdata, size_t *out_frames);
        FVM_Result (*pause)(void *,
                            int);    /* Atomically set paused state on OK; error preserves it. */
        FVM_Result (*clear)(void *); /* Discard queued frames on OK, without closing the device. */
    } FVM_AudioQueueBackend;
    /* Own pending PCM without owning the borrowed mixer or backend device. */
    typedef struct FVM_SWFSoundOutput FVM_SWFSoundOutput;
    /* Copied output state, refreshed only through explicit host operations. */
    typedef struct FVM_SWFSoundOutputInfo
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFSoundOutputInfo). */
        double submitted_frames;   /* Accepted frames since successful stop/reset, exact through
                                      MAX_CLOCK. */
        double played_frames; /* Accepted minus queued; queue-dequeue clock, not DAC position. */
        double seconds;       /* played_frames divided by the output sample rate. */
        size_t queued_frames; /* Last validated FIFO backlog. */
        size_t
            pending_frames; /* Prepared but not yet accepted frames retained after write failure. */
        int paused;         /* Last successful host pause state. */
        int starved; /* Nonpaused, previously submitted data, currently empty FIFO; no underrun
                        duration inference. */
    } FVM_SWFSoundOutputInfo;
    /* Borrow mixer/backend exclusively; require matching geometry and an empty queue. Success
     * pauses it. */
    FVM_API FVM_Result fvm_swf_sound_output_create(FVM_SWFSoundMixer *,
                                                   const FVM_AudioQueueBackend *,
                                                   size_t block_frames, size_t target_frames,
                                                   FVM_SWFSoundOutput **out);
#define FVM_SWF_SOUND_OUTPUT_ID_SLOTS 65536UL /* Fixed optional ID table, including unused ID zero. */
    /* Optional queued identity policy; configure only in an empty output epoch with no mixer
     * voices, accepted or pending PCM. enabled is 0/1. Preallocate ID_SLOTS doubles on enable;
     * disable frees the table. No callback/VM/device mutation; allocation failure leaves old
     * ownership intact. Invalid arguments/state INVALID_ARGUMENT; allocation/size failure OOM.
     * Retain/serialize output and borrowed mixer. Tracking requires ALL voice starts/stops to
     * use this output's tracked APIs; direct mixer mutation invalidates identity accounting. */
    FVM_API FVM_Result fvm_swf_sound_output_set_identity_tracking(FVM_SWFSoundOutput *, int enabled);
    /* Requested owned storage; excluded: borrowed mixer/backend, device, VM and allocator headers. */
    typedef struct FVM_SWFSoundOutputStorageInfo
    {
        size_t struct_size; /* Caller supplies exact sizeof this observation. */
        size_t fixed_bytes; /* Native output object and vector descriptors. */
        size_t pcm_bytes; /* Exact fixed pending/retry buffer. */
        size_t history_bytes; /* Optional exact accepted-PCM history ring. */
        size_t identity_bytes; /* Optional exact ID deadline table. */
        size_t retained_bytes; /* Checked sum of the four owned charges. */
    } FVM_SWFSoundOutputStorageInfo;
    /* Copy storage without allocation, callbacks or VM. Null/wrong size/reentry INVALID_ARGUMENT;
     * checked sum overflow OOM; errors preserve out. Externally serialize live output. */
    FVM_API FVM_Result fvm_swf_sound_output_storage_info(const FVM_SWFSoundOutput *,
                                                         FVM_SWFSoundOutputStorageInfo *);
    /* Observe backend backlog and test whether ID 1..65535 has not reached its exclusive end
     * on the accepted-minus-queued clock. Includes uncomposed/pending registered voice time;
     * not DAC audibility. Tracking disabled UNSUPPORTED. Invalid/reentry INVALID_ARGUMENT;
     * backend errors propagate, bad_alloc OOM, other exceptions ERROR; preserve out on failure.
     * No allocation/VM; callback may block per backend contract. Keep owners serialized/live. */
    FVM_API FVM_Result fvm_swf_sound_output_sound_active(FVM_SWFSoundOutput *, unsigned int id,
                                                         int *out);
    /* Copied identity diagnostics use FIFO dequeue time, including pending/uncomposed intervals. */
    typedef struct FVM_SWFSoundOutputIdentityInfo
    {
        size_t struct_size; /* Caller supplies exact sizeof this native observation. */
        unsigned int sound_id; /* Requested dictionary identity, 1..65535. */
        double played_frames; /* Observed accepted-minus-queued epoch cursor. */
        double end_frame; /* Last registered exclusive end; zero after clear/ID cancellation. */
        double until_end_frames; /* max(end_frame-played_frames,0); includes gaps before a new start. */
        int active; /* Nonzero exactly while until_end_frames is positive. */
    } FVM_SWFSoundOutputIdentityInfo;
    /* Observe backend and copy one identity's deadline/cursor without allocation, decode or VM.
     * Expired registrations retain their historical end until overwritten/cancelled; they are
     * inactive. until_end is time to the deadline, not a count of remaining nonzero PCM samples
     * or DAC latency. Invalid ID/null/wrong size/reentry INVALID_ARGUMENT; disabled UNSUPPORTED;
     * backend errors propagate, bad_alloc OOM, other exceptions ERROR. Errors preserve out.
     * Retain/serialize output/mixer/backend until callback return, as for sound_active. */
    FVM_API FVM_Result fvm_swf_sound_output_identity_info(FVM_SWFSoundOutput *, unsigned int id,
                                                          FVM_SWFSoundOutputIdentityInfo *out);
    /* Transfer a prepared voice with queued SyncNoMultiple semantics and bounded mixer storage.
     * Tracking must be enabled; disabled UNSUPPORTED. Observe the FIFO, include existing pending
     * PCM before the new start, and register the exact exclusive end at a fresh conversion phase.
     * NoMultiple consumes/closes/clears an unused voice while any registered same-ID interval
     * remains unplayed, even after mixer retirement. Ordinary same-ID starts overlap and extend
     * the maximum end. Finished voices add no interval. Success consumes/clears *voice; failures
     * preserve its ownership/progress and old voices/intervals (queue observation may refresh).
     * mixer_budget covers mixer/slots/voices only, as mixer_submit_prepared_bounded; output,
     * backend and host template storage are separate. Duration/epoch overflow UNSUPPORTED;
     * other mixer/backend statuses propagate; bad_alloc OOM, other exceptions ERROR.
     * No allocation, decode or VM. Serialize output/mixer/voice externally; callbacks may block.
     * Do not submit/read/stop the borrowed mixer independently once tracked starts are used.
     * Accepted and pending PCM are immutable; new voices begin at the next composition boundary. */
    FVM_API FVM_Result fvm_swf_sound_output_submit_prepared_bounded(FVM_SWFSoundOutput *,
                                                       FVM_SWFSoundVoice **voice, size_t mixer_budget);
    /* Stop future composition for ID 1..65535 with the mixer's immediate/DECLICK policy and
     * release that ID's NoMultiple registration. Does not change accepted/pending PCM, clear
     * the FIFO or rewind other voices. New same-ID NoMultiple is allowed after this command;
     * old queued/releasing PCM can still play. Missing ID OK. Tracking disabled UNSUPPORTED;
     * null/bad ID/reentry INVALID_ARGUMENT. No allocation, callback, VM or clock observation.
     * Keep output/mixer live and externally serialized; direct mixer stops bypass this ledger. */
    FVM_API FVM_Result fvm_swf_sound_output_stop_id(FVM_SWFSoundOutput *, unsigned int id);
    /* Prepare/retry one block. NOT_FOUND means no complete block fits below the queue target. */
    FVM_API FVM_Result fvm_swf_sound_output_pump(FVM_SWFSoundOutput *, size_t *accepted_frames);
    /* Finite variant: clamp only fresh PCM to the current mixer horizon and free FIFO
     * headroom. Empty voices -> NOT_FOUND without generating silence. Silent-valued
     * source/release frames still count. Pending PCM from either pump or stop_release
     * is indivisible and retried unchanged, even after all mixer voices retire.
     * accepted optional/cleared first. Same owner/backend/serialization, clock and
     * exception/status contract as pump; no allocation, VM, GC or implicit controls.
     * Legacy pump keeps continuous silent priming. NOT_FOUND is not queue drain or
     * physical audibility; observe pending/FIFO/voices separately before cleanup. */
    FVM_API FVM_Result fvm_swf_sound_output_pump_finite(FVM_SWFSoundOutput *, size_t *accepted);

    /* Set queue pause state; never implicitly primes or clears PCM. */
    FVM_API FVM_Result fvm_swf_sound_output_pause(FVM_SWFSoundOutput *, int paused);
    /* Query queue and copied clock; errors preserve caller output. */
    FVM_API FVM_Result fvm_swf_sound_output_info(FVM_SWFSoundOutput *, FVM_SWFSoundOutputInfo *);
    /* Pause, clear, stop software voices, discard pending PCM and reset this clock epoch.
     * Enabled identity registrations reset only after successful clear. A failed clear retains
     * old registrations/voices/pending; an acknowledged pause remains visible. */
    FVM_API FVM_Result fvm_swf_sound_output_stop(FVM_SWFSoundOutput *);
    /* Opt-in history allocation before first submission; frames=0 disables, otherwise 2..block. */
    FVM_API FVM_Result fvm_swf_sound_output_set_release(FVM_SWFSoundOutput *, size_t frames);
    /* Discard queued voices, then queue a short release from the observed consumed boundary.
     * Does not pause a running device; old immediate stop remains unchanged. Requires set_release.
     * Successful clear cancels every enabled identity, including if release write later fails.
     * A retained anonymous release retry does not reserve a sound ID; failed clear preserves IDs.
     */
    FVM_API FVM_Result fvm_swf_sound_output_stop_release(FVM_SWFSoundOutput *);
    /* Free pending PCM only; no callbacks, device stop/close or mixer ownership transfer. Null
     * accepted. */
    FVM_API void fvm_swf_sound_output_close(FVM_SWFSoundOutput *);
    /* Own one SDL2 device separately from the software output producer. */
    typedef struct FVM_SDLAudioQueue FVM_SDLAudioQueue;
#define FVM_SDL_AUDIO_MIN_BUFFER 64U    /* Smallest requested SDL callback buffer, in frames. */
#define FVM_SDL_AUDIO_MAX_BUFFER 32768U /* Largest power-of-two SDL buffer frames. */
    /* OS: Linux/Windows/macOS/Android via SDL2. Exact S16 geometry; native block size may change.
     * Starts paused. Disabled build returns UNSUPPORTED. */
    FVM_API FVM_Result fvm_audio_sdl_queue_open(unsigned int rate, unsigned int channels,
                                                unsigned int device_buffer_frames,
                                                FVM_SDLAudioQueue **out,
                                                FVM_AudioQueueBackend *backend);
    /* Immutable device facts; SDL may enlarge its requested callback block on Android. */
    typedef struct FVM_SDLAudioQueueInfo
    {
        unsigned long struct_size; /* Required sizeof(FVM_SDLAudioQueueInfo). */
        unsigned int sample_rate;  /* Actual native S16 frames per second. */
        unsigned int channels;     /* Actual native interleaved channels. */
        unsigned int
            device_buffer_frames; /* Obtained SDL callback quantum, not software FIFO depth. */
    } FVM_SDLAudioQueueInfo;
    /* Copy immutable obtained geometry without a callback or allocation; errors preserve output. */
    FVM_API FVM_Result fvm_audio_sdl_queue_info(const FVM_SDLAudioQueue *, FVM_SDLAudioQueueInfo *);
    /* Close the real device only after pump close. Null succeeds; no mixer/VM access. */
    FVM_API FVM_Result fvm_audio_sdl_queue_close(FVM_SDLAudioQueue *);
#ifdef __cplusplus
}
#endif
#endif
