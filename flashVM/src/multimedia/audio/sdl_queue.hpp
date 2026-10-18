/* OS: Linux/Windows/macOS SDL2 audio-device queue, enabled only with an explicit SDK option. */
#ifndef FVM_SDL_QUEUE_HPP /* Guard private native state and operation declarations. */
#define FVM_SDL_QUEUE_HPP /* Keep one native handle representation per translation unit. */
#include "flashvm/swf_sound_output.h" /* Reuse the C89 queue table and exact geometry limits. */
#define FVM_SDL_QUEUE_CHANNELS 2U /* Bound SDL queue input to native mono/stereo. */
#define FVM_SDL_QUEUE_SAMPLE_BYTES 2U /* SDL AUDIO_S16SYS stores two bytes per channel value. */
#define FVM_SDL_QUEUE_BYTE_BITS 8U /* SDK PCM byte accounting requires eight-bit bytes. */
#if defined(FVM_HAS_SDL_AUDIO) /* Default builds retain ABI stubs without requiring SDL headers. */
#define SDL_MAIN_HANDLED /* Native host entrypoints own main; do not link SDL2main implicitly. */
#include <SDL.h> /* Call the real installed SDL2 SDK. */
struct FVM_SDLAudioQueue /* Own device lifetime independently of mixer/context/pump. */
{ /* Begin native SDL state. */
    FVM_SDLAudioQueueInfo info; /* Immutable obtained geometry, including enlarged native callback quantum. */
    SDL_AudioDeviceID device; /* Own the explicit playback device until close. */
    bool faulted; /* A failed native write requires explicit clear/reset before trusting queue counts. */
    unsigned int frame_bytes; /* Native signed-short bytes per complete interleaved frame. */
}; /* Finish device state. */
FVM_Result fvm_sdl_queue_write(void *, const short *, size_t); /* OS: SDL2 QueueAudio copies PCM atomically. */
FVM_Result fvm_sdl_queue_queued(void *, size_t *); /* OS: SDL2 reports bytes still in its software FIFO. */
FVM_Result fvm_sdl_queue_pause(void *, int); /* OS: SDL2 pauses/resumes the native playback thread. */
FVM_Result fvm_sdl_queue_clear(void *); /* OS: SDL2 drops pending software-queue data. */
#endif /* End SDK-only declarations. */
#endif /* End private interface. */
