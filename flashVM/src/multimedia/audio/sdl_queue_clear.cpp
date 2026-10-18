/* OS: Linux/Windows/macOS SDL2 pending software-queue discard. */
#include "sdl_queue.hpp" /* Use the actual native device handle. */
#if defined(FVM_HAS_SDL_AUDIO) /* Disabled builds publish no callable SDL backend. */
/* Discard copied FIFO data; already handed-off hardware buffers remain outside this observation. */
FVM_Result fvm_sdl_queue_clear(void *raw) /* The pump pauses first when resetting its clock epoch. */
{ /* Begin explicit destructive queue operation. */
    FVM_SDLAudioQueue *queue = (FVM_SDLAudioQueue *)raw; /* Borrow backend ownership. */
    if (!queue) return FVM_INVALID_ARGUMENT; /* Do not call SDL with an absent handle. */
    if (SDL_GetAudioDeviceStatus(queue->device) == SDL_AUDIO_STOPPED) return FVM_ERROR; /* Do not hide device loss. */
    SDL_ClearQueuedAudio(queue->device); queue->faulted = false; return FVM_OK; /* SDL discards all remaining software FIFO bytes. */
} /* End FIFO discard without device close. */
#endif /* End enabled backend operation. */
