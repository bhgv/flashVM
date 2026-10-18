/* FlashVM: explicit serialized native PCM queue pumping and dequeue-clock observation. */
#ifndef FLASHVM_SWF_SOUND_OUTPUT_H
#define FLASHVM_SWF_SOUND_OUTPUT_H /* Guard the independent C89 queue/output ABI. */
#include "swf_sound_mixer.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Backend table version and independent resource/accounting bounds. */
#define FVM_AUDIO_QUEUE_ABI_VERSION 1UL /* Exact native callback-table ABI. */
#define FVM_SWF_SOUND_OUTPUT_MAX_BLOCK 65536UL /* Frames in one preparation/retry buffer. */
#define FVM_SWF_SOUND_OUTPUT_MAX_QUEUE 262144UL /* Maximum requested FIFO frames. */
#define FVM_SWF_SOUND_OUTPUT_CLOCK_BITS 53 /* Required double mantissa precision. */
#define FVM_SWF_SOUND_OUTPUT_MAX_CLOCK 9007199254740991.0 /* Exact frame accounting limit. */
/* Copied native callback table; the host retains ownership of userdata/device. */
typedef struct FVM_AudioQueueBackend
{
    unsigned long struct_size; /* Required sizeof(FVM_AudioQueueBackend). */
    unsigned long abi_version; /* Required FVM_AUDIO_QUEUE_ABI_VERSION. */
    unsigned int sample_rate;  /* Actual queue input clock, in frames per second. */
    unsigned int channels;     /* Actual queue input channels, native interleaved S16. */
    void *userdata;            /* Borrowed until output close; backend/device remains host-owned. */
    /* Copy all frames before OK; accept none on any error/exception. */
    FVM_Result (*write)(void *userdata, const short *pcm, size_t frames);
    /* Report complete FIFO frames, including while paused; preserve out on error. */
    FVM_Result (*queued)(void *userdata, size_t *out_frames);
    FVM_Result (*pause)(void *, int); /* Atomically set paused state on OK; error preserves it. */
    FVM_Result (*clear)(void *);      /* Discard queued frames on OK, without closing the device. */
} FVM_AudioQueueBackend;
/* Own pending PCM without owning the borrowed mixer or backend device. */
typedef struct FVM_SWFSoundOutput FVM_SWFSoundOutput;
/* Copied output state, refreshed only through explicit host operations. */
typedef struct FVM_SWFSoundOutputInfo
{
    unsigned long struct_size; /* Required sizeof(FVM_SWFSoundOutputInfo). */
    double submitted_frames;   /* Accepted frames since successful stop/reset, exact through
                                  MAX_CLOCK. */
    double played_frames;      /* Accepted minus queued; queue-dequeue clock, not DAC position. */
    double seconds;            /* played_frames divided by the output sample rate. */
    size_t queued_frames;      /* Last validated FIFO backlog. */
    size_t pending_frames; /* Prepared but not yet accepted frames retained after write failure. */
    int paused;            /* Last successful host pause state. */
    int starved; /* Nonpaused, previously submitted data, currently empty FIFO; no underrun
                    duration inference. */
} FVM_SWFSoundOutputInfo;
/* Borrow mixer/backend exclusively; require matching geometry and an empty queue. Success
 * pauses it. */
FVM_API FVM_Result fvm_swf_sound_output_create(FVM_SWFSoundMixer *, const FVM_AudioQueueBackend *,
                                            size_t block_frames, size_t target_frames,
                                            FVM_SWFSoundOutput **out);
/* Prepare/retry one block. NOT_FOUND means no complete block fits below the queue target. */
FVM_API FVM_Result fvm_swf_sound_output_pump(FVM_SWFSoundOutput *, size_t *accepted_frames);
/* Set queue pause state; never implicitly primes or clears PCM. */
FVM_API FVM_Result fvm_swf_sound_output_pause(FVM_SWFSoundOutput *, int paused);
/* Query queue and copied clock; errors preserve caller output. */
FVM_API FVM_Result fvm_swf_sound_output_info(FVM_SWFSoundOutput *, FVM_SWFSoundOutputInfo *);
/* Pause, clear, stop software voices, discard pending PCM and reset this clock epoch. */
FVM_API FVM_Result fvm_swf_sound_output_stop(FVM_SWFSoundOutput *);
/* Free pending PCM only; no callbacks, device stop/close or mixer ownership transfer. Null
 * accepted. */
FVM_API void fvm_swf_sound_output_close(FVM_SWFSoundOutput *);
/* Own one SDL2 device separately from the software output producer. */
typedef struct FVM_SDLAudioQueue FVM_SDLAudioQueue;
#define FVM_SDL_AUDIO_MIN_BUFFER 64U /* Smallest requested SDL callback buffer, in frames. */
#define FVM_SDL_AUDIO_MAX_BUFFER 32768U /* Largest power-of-two SDL buffer frames. */
/* OS: Linux/Windows/macOS/Android via SDL2. Exact S16 geometry; native block size may change. Starts paused. Disabled build returns
 * UNSUPPORTED. */
FVM_API FVM_Result fvm_audio_sdl_queue_open(unsigned int rate, unsigned int channels,
                                            unsigned int device_buffer_frames,
                                            FVM_SDLAudioQueue **out,
                                            FVM_AudioQueueBackend *backend);
/* Immutable device facts; SDL may enlarge its requested callback block on Android. */
typedef struct FVM_SDLAudioQueueInfo
{
    unsigned long struct_size; /* Required sizeof(FVM_SDLAudioQueueInfo). */
    unsigned int sample_rate; /* Actual native S16 frames per second. */
    unsigned int channels; /* Actual native interleaved channels. */
    unsigned int device_buffer_frames; /* Obtained SDL callback quantum, not software FIFO depth. */
} FVM_SDLAudioQueueInfo;
/* Copy immutable obtained geometry without a callback or allocation; errors preserve output. */
FVM_API FVM_Result fvm_audio_sdl_queue_info(const FVM_SDLAudioQueue *, FVM_SDLAudioQueueInfo *);
/* Close the real device only after pump close. Null succeeds; no mixer/VM access. */
FVM_API FVM_Result fvm_audio_sdl_queue_close(FVM_SDLAudioQueue *);
#ifdef __cplusplus
}
#endif
#endif
