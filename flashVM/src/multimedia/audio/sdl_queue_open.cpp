/* OS: Linux/Windows/macOS SDL2, explicit native S16 playback queue creation. */
#include "sdl_queue.hpp" /* Import the replaceable queue ABI and optional actual SDK. */
#include <cstring>       /* Clear published outputs and SDL specifications before use. */
#include <climits>       /* Check narrowing to SDL's signed integer sample-rate field. */
#include <new>           /* Return allocation failure without publishing a device owner. */

/* Open an exact-format, initially paused playback device; never fall back to dummy silently. */
extern "C" FVM_Result fvm_audio_sdl_queue_open(unsigned int rate, unsigned int channels,                                      /* Requested PCM geometry. */
                                               unsigned int samples, FVM_SDLAudioQueue **out, FVM_AudioQueueBackend *backend) /* Separate owned handle/table outputs. */
{                                                                                                                             /* Begin native resource acquisition. */
    if (out)                                                                                                                  /* Condition for this action: Preserve atomic handle publication on every failure. */
    {                                                                                                                         /* Begin conditional branch: Condition for this action: Preserve atomic handle publication on every failure. */
        *out = 0;                                                                                                             /* Preserve atomic handle publication on every failure. */
    } /* End conditional branch: Condition for this action: Preserve atomic handle publication on every failure. */
    if (backend)                                   /* Condition for this action: Never return a partial callback table. */
    {                                              /* Begin conditional branch: Condition for this action: Never return a partial callback table. */
        std::memset(backend, 0, sizeof(*backend)); /* Never return a partial callback table. */
    } /* End conditional branch: Condition for this action: Never return a partial callback table. */
    if (!out || !backend || rate < FVM_SWF_SOUND_MIXER_MIN_RATE || rate > FVM_SWF_SOUND_MIXER_MAX_RATE ||                       /* Bound rate before casts. */
        rate > (unsigned int)INT_MAX || !channels || channels > FVM_SDL_QUEUE_CHANNELS || samples < FVM_SDL_AUDIO_MIN_BUFFER || /* Require mono/stereo. */
        samples > FVM_SDL_AUDIO_MAX_BUFFER || (samples & (samples - 1)))                                                        /* SDL callback frames must be a bounded power of two. */
    {                                                                                                                           /* Begin scope: SDL callback frames must be a bounded power of two. */
        return FVM_INVALID_ARGUMENT;                                                                                            /* No subsystem/device is acquired for invalid input. */
    } /* End scope: SDL callback frames must be a bounded power of two. */
    if (CHAR_BIT != FVM_SDL_QUEUE_BYTE_BITS || sizeof(short) != FVM_SDL_QUEUE_SAMPLE_BYTES) /* Require exact native S16 storage. */
    {                                                                                       /* Begin conditional branch: Require exact native S16 storage. */
        return FVM_UNSUPPORTED;                                                             /* Never reinterpret a wider/narrower host short as SDL PCM. */
    } /* End conditional branch: Require exact native S16 storage. */
#if defined(FVM_HAS_SDL_AUDIO)                  /* Only explicitly enabled builds use the installed native SDK. */
    SDL_SetMainReady();                         /* The host owns its entrypoint, including on Windows. */
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) /* Condition for this action: Acquire one balanced audio-subsystem reference. */
    {                                           /* Begin conditional branch: Condition for this action: Acquire one balanced audio-subsystem reference. */
        return FVM_ERROR;                       /* Acquire one balanced audio-subsystem reference. */
    } /* End conditional branch: Condition for this action: Acquire one balanced audio-subsystem reference. */
    SDL_AudioSpec desired, obtained;                                                                           /* Retain both requested and actual queue input geometry. */
    std::memset(&desired, 0, sizeof(desired));                                                                 /* Null callback selects SDL's copy-based queue mode. */
    std::memset(&obtained, 0, sizeof(obtained));                                                               /* Never inspect fields from a failed open. */
    desired.freq = (int)rate;                                                                                  /* Express output frames per second. */
    desired.channels = (Uint8)channels;                                                                        /* Require complete mono/stereo interleaving. */
    desired.format = AUDIO_S16SYS;                                                                             /* Preserve native-endian signed-short mixer output. */
    desired.samples = (Uint16)samples;                                                                         /* Select the bounded SDK playback buffer. */
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(0, 0, &desired, &obtained, SDL_AUDIO_ALLOW_SAMPLES_CHANGE); /* Expose actual native quantum without permitting format/rate/channel changes. */
    if (!device)                                                                                               /* Condition for this action: Balance initialization on native failure. */
    {                                                                                                          /* Begin conditional branch: Condition for this action: Balance initialization on native failure. */
        SDL_QuitSubSystem(SDL_INIT_AUDIO);                                                                     /* Balance initialization on native failure. */
        return FVM_ERROR;                                                                                      /* Balance initialization on native failure. */
    } /* Balance initialization on native failure. */
    if (!obtained.samples || obtained.freq != desired.freq || obtained.channels != desired.channels || obtained.format != desired.format) /* Verify SDK contract. */
    {                                                                                                                                     /* Begin conditional branch: Verify SDK contract. */
        SDL_CloseAudioDevice(device);                                                                                                     /* Reject unexpected geometry. */
        SDL_QuitSubSystem(SDL_INIT_AUDIO);                                                                                                /* Reject unexpected geometry. */
        return FVM_UNSUPPORTED;                                                                                                           /* Reject unexpected geometry. */
    } /* Reject unexpected geometry. */
    FVM_SDLAudioQueue *queue = new (std::nothrow) FVM_SDLAudioQueue; /* Allocate only after acquiring a real device. */
    if (!queue)                                                      /* Condition for this action: Release partial resources. */
    {                                                                /* Begin conditional branch: Condition for this action: Release partial resources. */
        SDL_CloseAudioDevice(device);                                /* Release partial resources. */
        SDL_QuitSubSystem(SDL_INIT_AUDIO);                           /* Release partial resources. */
        return FVM_OUT_OF_MEMORY;                                    /* Release partial resources. */
    } /* Release partial resources. */
    queue->info.struct_size = sizeof(queue->info);               /* Publish a complete immutable device-facts layout. */
    queue->info.sample_rate = (unsigned int)obtained.freq;       /* Retain actual input geometry. */
    queue->info.channels = obtained.channels;                    /* Retain actual input geometry. */
    queue->info.device_buffer_frames = obtained.samples;         /* Android may enlarge this beyond desired.samples. */
    queue->faulted = false;                                      /* Start with trustworthy native FIFO accounting. */
    queue->device = device;                                      /* Transfer native device ownership to the unpublished handle. */
    queue->frame_bytes = channels * (unsigned int)sizeof(short); /* Store complete-frame byte accounting. */
    backend->struct_size = sizeof(*backend);                     /* Publish the exact C89 queue-table version. */
    backend->abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;          /* Match pump validation. */
    backend->sample_rate = rate;                                 /* Report exact accepted input geometry. */
    backend->channels = channels;                                /* Report exact accepted input geometry. */
    backend->userdata = queue;                                   /* Borrow this owner until the host closes it after the pump. */
    backend->write = fvm_sdl_queue_write;                        /* Install independent operation adapters. */
    backend->queued = fvm_sdl_queue_queued;                      /* Install independent operation adapters. */
    backend->pause = fvm_sdl_queue_pause;                        /* Separate pause from destructive discard. */
    backend->clear = fvm_sdl_queue_clear;                        /* Separate pause from destructive discard. */
    *out = queue;                                                /* SDL opens paused; publish only complete ownership/table state. */
    return FVM_OK;                                               /* SDL opens paused; publish only complete ownership/table state. */
#else                                                            /* Keep the public ABI usable without an SDL SDK dependency. */
    return FVM_UNSUPPORTED; /* Do not substitute a fake audio implementation. */
#endif                                                           /* Finish enabled/disabled backend selection. */
} /* End exact-format native queue creation. */
