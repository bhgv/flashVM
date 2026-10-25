/* OS: Linux/Windows/macOS SDL2 software-FIFO backlog observation. */
#include "sdl_queue.hpp"       /* Import complete-frame byte accounting and native device state. */
#if defined(FVM_HAS_SDL_AUDIO) /* Never infer native progress in a disabled build. */


/* Report queue-dequeue progress, not sound-card/DAC latency or a wall clock. */
FVM_Result fvm_sdl_queue_queued(void *raw, size_t *frames) /* Preserve caller output on failure. */
{                                                          /* Begin synchronous FIFO observation. */
    FVM_SDLAudioQueue *queue = (FVM_SDLAudioQueue *)raw;   /* Borrow the still-live output handle. */
    if (!queue || !frames)                                 /* Condition for this action: Require complete output storage. */
    {                                                      /* Begin conditional branch: Condition for this action: Require complete output storage. */
        return FVM_INVALID_ARGUMENT;                       /* Require complete output storage. */
    } /* End conditional branch: Condition for this action: Require complete output storage. */
    if (SDL_GetAudioDeviceStatus(queue->device) == SDL_AUDIO_STOPPED) /* Condition for this action: Do not count a lost device as played audio. */
    {                                                                 /* Begin conditional branch: Condition for this action: Do not count a lost device as played audio. */
        return FVM_ERROR;                                             /* Do not count a lost device as played audio. */
    } /* End conditional branch: Condition for this action: Do not count a lost device as played audio. */
    if (queue->faulted)   /* Condition for this action: A failed QueueAudio may leave SDK queue counts unreliable until clear. */
    {                     /* Begin conditional branch: Condition for this action: A failed QueueAudio may leave SDK queue counts unreliable until clear. */
        return FVM_ERROR; /* A failed QueueAudio may leave SDK queue counts unreliable until clear. */
    } /* End conditional branch: Condition for this action: A failed QueueAudio may leave SDK queue counts unreliable until clear. */
    Uint32 bytes = SDL_GetQueuedAudioSize(queue->device); /* Read SDL's actual copied-byte FIFO. */
    if (bytes % queue->frame_bytes)                       /* Condition for this action: Reject partial-frame accounting rather than round time. */
    {                                                     /* Begin conditional branch: Condition for this action: Reject partial-frame accounting rather than round time. */
        return FVM_ERROR;                                 /* Reject partial-frame accounting rather than round time. */
    } /* End conditional branch: Condition for this action: Reject partial-frame accounting rather than round time. */
    *frames = bytes / queue->frame_bytes; /* Publish complete sample frames only. */
    return FVM_OK;                        /* Publish complete sample frames only. */
} /* End native backlog query. */
#endif /* End SDK-only operation. */
