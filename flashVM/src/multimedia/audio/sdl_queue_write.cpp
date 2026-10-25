/* OS: Linux/Windows/macOS SDL2 complete-block PCM copying. */
#include "sdl_queue.hpp"       /* Use private device state and the actual optional SDK. */
#include <climits>             /* Bound SDL's Uint32 byte-count narrowing. */
#if defined(FVM_HAS_SDL_AUDIO) /* The operation exists only behind a real enabled backend table. */


/* Copy all frames or report rejection without consuming the caller's pending block. */
FVM_Result fvm_sdl_queue_write(void *raw, const short *pcm, size_t frames)    /* Borrow data only during QueueAudio. */
{                                                                             /* Begin complete-block validation. */
    FVM_SDLAudioQueue *queue = (FVM_SDLAudioQueue *)raw;                      /* Recover the exclusively host-owned device. */
    if (!queue || (frames && !pcm) || frames > UINT_MAX / queue->frame_bytes) /* Condition for this action: Avoid overflow/narrowing. */
    {                                                                         /* Begin conditional branch: Condition for this action: Avoid overflow/narrowing. */
        return FVM_INVALID_ARGUMENT;                                          /* Avoid overflow/narrowing. */
    } /* End conditional branch: Condition for this action: Avoid overflow/narrowing. */
    if (SDL_GetAudioDeviceStatus(queue->device) == SDL_AUDIO_STOPPED) /* Condition for this action: Device loss must not become clock progress. */
    {                                                                 /* Begin conditional branch: Condition for this action: Device loss must not become clock progress. */
        return FVM_ERROR;                                             /* Device loss must not become clock progress. */
    } /* End conditional branch: Condition for this action: Device loss must not become clock progress. */
    if (queue->faulted)   /* Condition for this action: Require explicit reset after a native allocation/write failure. */
    {                     /* Begin conditional branch: Condition for this action: Require explicit reset after a native allocation/write failure. */
        return FVM_ERROR; /* Require explicit reset after a native allocation/write failure. */
    } /* End conditional branch: Condition for this action: Require explicit reset after a native allocation/write failure. */
    if (!frames)       /* Condition for this action: Empty writes do not require a nonnull PCM buffer. */
    {                  /* Begin conditional branch: Condition for this action: Empty writes do not require a nonnull PCM buffer. */
        return FVM_OK; /* Empty writes do not require a nonnull PCM buffer. */
    } /* End conditional branch: Condition for this action: Empty writes do not require a nonnull PCM buffer. */
    if (SDL_QueueAudio(queue->device, pcm, (Uint32)(frames * queue->frame_bytes)) != 0) /* Copy or reject the complete native block. */
    {                                                                                   /* Do not trust SDL FIFO bookkeeping after a native allocation failure. */
        queue->faulted = true;                                                          /* Prevent erroneous queue depletion from being counted as played frames. */
        return FVM_ERROR;                                                               /* Retain producer PCM until explicit stop/reset or close. */
    } /* Finish failure isolation without automatic replay. */
    return FVM_OK; /* Native queue owns its independent complete PCM copy. */
} /* End copied-buffer ownership. */
#endif /* End SDK-only operation. */
