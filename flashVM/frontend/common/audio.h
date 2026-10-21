/* Serialized native audio worker ownership; VM/event/render operations stay on the main thread. */
#ifndef FVM_DESKTOP_AUDIO_H
#define FVM_DESKTOP_AUDIO_H
#include "flashvm/swf_sound_output.h"
#include "flashvm/audio_stop.h"
#include "flashvm/swf_stream_output.h"
#include <SDL.h>
#include "audio_trace.h"
#include "audio_memory.h"
#define FVM_DESKTOP_AUDIO_RELEASE_DIVISOR 200U /* Five-millisecond native stop release. */
#define FVM_DESKTOP_AUDIO_RATE 48000U          /* Native frame clock, independent of movie FPS. */
#define FVM_DESKTOP_AUDIO_CHANNELS 2U          /* Stereo native S16. */
#define FVM_DESKTOP_AUDIO_VOICES 32U           /* Maximum overlapping prepared voices. */
#define FVM_DESKTOP_AUDIO_BLOCK 1024U          /* Bounded preparation buffer in frames. */
#define FVM_DESKTOP_AUDIO_MIN_TARGET 4800U     /* At least 100 ms of scheduling reserve. */
#define FVM_DESKTOP_AUDIO_QUANTA 4U /* Keep at least four obtained driver blocks in reserve. */
#define FVM_DESKTOP_AUDIO_RESUME_QUANTA 2U /* Prime at least two native blocks before resume. */
#define FVM_DESKTOP_AUDIO_PUMPS 8U         /* Bound work per worker iteration. */
#define FVM_DESKTOP_AUDIO_WAIT 5 /* Worker interval and responsive main-thread event wait, ms. */
#define FVM_DESKTOP_AUDIO_TAIL_MS                                                                  \
    2000UL /* Extra finite-run completion budget after the final frame deadline. */
#define FVM_DESKTOP_AUDIO_MAX_SEGMENTS                                                             \
    64U /* Bound owner-side snapshot preparation and seek work. */

/* Native immutable source ownership and an independently resettable conversion grid per header. */
typedef struct FVM_DesktopStreamSegment
{
    FVM_SWFStreamSoundSnapshot *source; /* Owned copied segment; released after worker join. */
    FVM_SWFStreamConverter converter;   /* Exclusively bound/worker or inactive under same mutex. */
    unsigned int frame; /* Header's one-based movie frame; source order breaks same-frame ties. */
} FVM_DesktopStreamSegment;
typedef struct FVM_DesktopAudio
{
    FVM_SWFStreamSoundSnapshot
        *stream_source; /* Borrowed first-source alias; nonnull stream flag. */
    FVM_DesktopStreamSegment *stream_segments; /* Owned bounded array, including partial init. */
    size_t stream_count; /* Initialized array length; null source entries are safe to close. */
    size_t stream_index; /* Current output binding, advanced only under mutex. */
    FVM_SWFStreamOutput stream_output; /* Caller-buffered native producer, valid with source. */
    short stream_pcm[FVM_DESKTOP_AUDIO_BLOCK * FVM_DESKTOP_AUDIO_CHANNELS]; /* Owned retry bytes. */
    unsigned int stream_frame;    /* Mutex-protected authorized entered movie frame, zero holds. */
    unsigned int stream_selected; /* Last observed intrinsic selection, owner writes under lock. */
    unsigned long stream_navigation; /* Last observed explicit seek token, scoped to this root. */
    int stream_entered;              /* Prior entry flag, used to detect pending navigation. */
    int stream_movie_paused;  /* Native clip stop flag, independent of foreground/session pause. */
    int stream_waiting; /* Prior native observation was a frame gate/EOF or explicit pause. */
    int stream_cancelled;     /* StopSounds holds this stream until a detected navigation reset. */
    FVM_SWFSoundMixer *mixer; /* Owned native voices; every access is serialized by mutex. */
    FVM_SWFSoundOutput *output; /* Owned PCM producer, serialized by mutex. */
    FVM_SDLAudioQueue *device;  /* Owned native output, closed after joining the worker. */
    FVM_AudioStopBinding
        *stop_binding;            /* Context registration; detach before releasing userdata. */
    FVM_DesktopAudioTrace *trace; /* Optional bounded evidence; saved only after worker shutdown. */
    FVM_AudioQueueBackend
        queue_backend;      /* Real backend behind the optional PCM evidence wrapper. */
    SDL_Thread *thread;     /* Owned native-only producer thread; never touches a VM/context. */
    SDL_mutex *mutex;       /* Serialize source submission, producer pumping, stops and pause. */
    SDL_atomic_t running;   /* Publish shutdown independently of the worker's mutex scope. */
    SDL_atomic_t error;     /* Latched FVM_Result, safely readable after native lock failure. */
    SDL_atomic_t underruns; /* Observed event/authorized-stream empty-playing transitions, saturated for streams. */
    int suspended;          /* Protected foreground/background pause state. */
    int was_starved;        /* Protected event/stream starvation edge; no inferred DAC duration. */
    size_t target_frames;   /* Immutable queue reserve derived from actual driver quantum. */
    size_t resume_frames;   /* Immutable minimum backlog before unpausing. */
    unsigned long events;   /* Main-thread successful real frame-entry controls. */
    unsigned long stops;    /* Main-thread successful context StopSounds controls. */
} FVM_DesktopAudio;
#ifdef __cplusplus
extern "C"
{
#endif
    /* Validate supported unmuted root stream policy before native presentation/device acquisition.
     * Up to MAX_SEGMENTS PCM, whole-packet ADPCM or supported MP3 headers without root StartSound; other
     * codec/mixed/excess UNSUPPORTED. Owner only; may allocate/copy/decode temporary sources; no
     * device or callbacks. Core stream alignment errors propagate. */
    FVM_Result fvm_desktop_audio_validate(FVM_Context *);
    /* Observe intrinsic root frame/play/entry on owner, then publish scalars under worker mutex.
     * Changed navigation token or pending/backward/skipped frame clears old FIFO and resets phase;
     * repeated observations do not rewind. No VM pointer reaches worker. Host pause remains
     * independent. */
    FVM_Result fvm_desktop_audio_timeline(FVM_Context *, FVM_Object *, FVM_DesktopAudio *);
    /* Acquire native ownership for root events or supported prepared root streams; never both.
     * Event preparation runs on owner outside worker mutex; aggregate native mixer/voice storage
     * obeys EVENT_BYTES. Native output/device/trace storage and VM memory are separate budgets. */
    FVM_Result fvm_desktop_audio_open(FVM_Context *, FVM_Object *, FVM_DesktopAudio *);
    /* Set initial suspension before starting the native PCM producer thread. */
    FVM_Result fvm_desktop_audio_open_paused(FVM_Context *, FVM_Object *, FVM_DesktopAudio *, int);
    /* Poll worker errors and mark the host foreground; never perform PCM work on the render thread.
     */
    FVM_Result fvm_desktop_audio_tick(FVM_DesktopAudio *);
    /* Poll the worker error latch while preserving an explicit owner-requested pause. */
    FVM_Result fvm_desktop_audio_tick_paused(FVM_DesktopAudio *, int paused);
    /* Serialize an explicit background pause; resume is primed by the worker. */
    FVM_Result fvm_desktop_audio_pause(FVM_DesktopAudio *, int paused);
    /* Copy queue observation under the same native mutex; useful for diagnostics and regression
     * tests. */
    FVM_Result fvm_desktop_audio_info(FVM_DesktopAudio *, FVM_SWFSoundOutputInfo *);
    /* Stream-only copied observation under producer mutex; invalid/wrong-size preserves output. */
    FVM_Result fvm_desktop_audio_stream_info(FVM_DesktopAudio *, FVM_SWFStreamOutputInfo *);
    /* Copy whether a running stream has authorized PCM/markers or accepted queued frames left.
     * Inactive future sources are inspected from their initial block; no decoding or VM access.
     * No audio, suspension, movie stop or cancellation reports zero. Query is mutex-serialized;
     * errors preserve output. Owner must retain initialized audio throughout the call. */
    FVM_Result fvm_desktop_audio_stream_pending(FVM_DesktopAudio *, int *);
    /* Owner safe point: detach sound delivery and join/release old root audio while keeping
     * cumulative counters. Context/window remain alive. Repeated clearing is safe. */
    FVM_Result fvm_desktop_audio_clear(FVM_Context *, FVM_DesktopAudio *);
    /* Detach, signal/join, stop/free native dependencies and destroy the mutex before SDL_Quit. */
    void fvm_desktop_audio_close(FVM_DesktopAudio *);
#ifdef __cplusplus
}
#endif
#endif
