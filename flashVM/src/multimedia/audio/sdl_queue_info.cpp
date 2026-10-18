/* OS: Linux/Windows/macOS/Android SDL2 immutable obtained device geometry. */
#include "sdl_queue.hpp" /* Import the copied public info ABI and actual optional device owner. */
/* Never infer callback quantum from the requested size; native drivers may enlarge it. */
FVM_Result fvm_audio_sdl_queue_info(const FVM_SDLAudioQueue *queue, FVM_SDLAudioQueueInfo *info) /* Borrow live owner and caller output. */
{ /* Begin immutable geometry publication. */
    if (!queue || !info || info->struct_size != sizeof(*info)) return FVM_INVALID_ARGUMENT; /* Preserve output on invalid input. */
#if defined(FVM_HAS_SDL_AUDIO) /* Copy actual SDK facts only from a successfully opened device. */
    *info = queue->info; return FVM_OK; /* No native query, allocation or VM pointer is required. */
#else /* A disabled build cannot produce a valid SDL device owner. */
    return FVM_UNSUPPORTED; /* Never fabricate device geometry. */
#endif /* End SDK selection. */
} /* End copied immutable device facts. */
