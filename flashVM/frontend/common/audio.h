/* Serialized native audio worker ownership; VM/event/render operations stay on the main thread. */
#ifndef FVM_DESKTOP_AUDIO_H
#define FVM_DESKTOP_AUDIO_H
#include "flashvm/swf_sound_output.h"
#include "flashvm/audio_stop.h"
#include <SDL.h>
#include "audio_trace.h"
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
typedef struct FVM_DesktopAudio
{
    FVM_SWFSoundMixer *mixer;   /* Owned native voices; every access is serialized by mutex. */
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
    SDL_atomic_t underruns; /* Count observed transitions into an empty playing FIFO. */
    int suspended;          /* Protected foreground/background pause state. */
    int was_starved;        /* Protected edge detector, not elapsed underrun duration. */
    size_t target_frames;   /* Immutable queue reserve derived from actual driver quantum. */
    size_t resume_frames;   /* Immutable minimum backlog before unpausing. */
    unsigned long events;   /* Main-thread successful real frame-entry controls. */
    unsigned long stops;    /* Main-thread successful context StopSounds controls. */
} FVM_DesktopAudio;
#ifdef __cplusplus
extern "C"
{
#endif
    /* Acquire native ownership and start a producer only when the file has root sound controls. */
    FVM_Result fvm_desktop_audio_open(FVM_Context *, FVM_Object *, FVM_DesktopAudio *);
    /* Poll worker errors and mark the host foreground; never perform PCM work on the render thread.
     */
    FVM_Result fvm_desktop_audio_tick(FVM_DesktopAudio *);
    /* Serialize an explicit background pause; resume is primed by the worker. */
    FVM_Result fvm_desktop_audio_pause(FVM_DesktopAudio *, int paused);
    /* Copy queue observation under the same native mutex; useful for diagnostics and regression
     * tests. */
    FVM_Result fvm_desktop_audio_info(FVM_DesktopAudio *, FVM_SWFSoundOutputInfo *);
    /* Owner safe point: detach sound delivery and join/release old root audio while keeping
     * cumulative counters. Context/window remain alive. Repeated clearing is safe. */
    FVM_Result fvm_desktop_audio_clear(FVM_Context *, FVM_DesktopAudio *);
    /* Detach, signal/join, stop/free native dependencies and destroy the mutex before SDL_Quit. */
    void fvm_desktop_audio_close(FVM_DesktopAudio *);
#ifdef __cplusplus
}
#endif
#endif
