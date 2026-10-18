/* OS: Linux/Windows/macOS SDL2 balanced playback-device/subsystem release. */
#include "sdl_queue.hpp" /* Import the opaque owner and optional real SDK. */
/* Close after pump teardown; no mixer/context callbacks or hidden ownership transfer. */
extern "C" FVM_Result fvm_audio_sdl_queue_close(FVM_SDLAudioQueue *queue) /* Null is an ordinary no-op. */
{ /* Begin final host-owned native cleanup. */
    if (!queue) return FVM_OK; /* Support partially initialized host cleanup. */
#if defined(FVM_HAS_SDL_AUDIO) /* Release only handles created by the enabled implementation. */
    SDL_CloseAudioDevice(queue->device); /* Wait for SDK playback use to finish and close the OS output. */
    delete queue; /* Invalidate native userdata after the playback thread is finished. */
    SDL_QuitSubSystem(SDL_INIT_AUDIO); return FVM_OK; /* Balance this owner's initialization reference only. */
#else /* No nonnull handle can be created by a disabled implementation. */
    return FVM_UNSUPPORTED; /* Do not pretend to close an unsupported foreign handle. */
#endif /* Finish enabled/disabled cleanup selection. */
} /* End native device ownership. */
