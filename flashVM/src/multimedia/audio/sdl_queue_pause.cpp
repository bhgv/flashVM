/* OS: Linux/Windows/macOS SDL2 playback-thread pause/resume. */
#include "sdl_queue.hpp" /* Keep native ownership separate from VM and pump state. */
#if defined(FVM_HAS_SDL_AUDIO) /* Compile only when the real SDK is explicitly selected. */
/* Pause/resume without clearing accepted PCM or the producer's pending retry block. */
FVM_Result fvm_sdl_queue_pause(void *raw, int paused) /* Normalize no implicit third state. */
{ /* Begin native state transition. */
    FVM_SDLAudioQueue *queue = (FVM_SDLAudioQueue *)raw; /* Borrow the live device. */
    if (!queue || (paused != 0 && paused != 1)) return FVM_INVALID_ARGUMENT; /* Validate before native mutation. */
    if (SDL_GetAudioDeviceStatus(queue->device) == SDL_AUDIO_STOPPED) return FVM_ERROR; /* Preserve device-loss reporting. */
    if (queue->faulted && !paused) return FVM_ERROR; /* Allow safety pause/reset but forbid resuming uncertain native state. */
    SDL_PauseAudioDevice(queue->device, paused); return FVM_OK; /* SDL synchronizes this thread state internally. */
} /* End nondestructive queue control. */
#endif /* End enabled backend operation. */
