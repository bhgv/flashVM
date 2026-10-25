/* Exercise production open with a real SDL dummy device behind a negotiating SDK shim. */
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include "flashvm/swf_sound_output.h"

#if defined(FVM_HAS_SDL_AUDIO)
/* Compile the actual bridge acquisition path under a distinct exported fixture name. */
#define fvm_audio_sdl_queue_open fixture_queue_open
#define SDL_OpenAudioDevice fixture_native_open
#include "../src/multimedia/audio/sdl_queue_open.cpp"

#undef SDL_OpenAudioDevice
#undef fvm_audio_sdl_queue_open
extern "C" SDL_AudioDeviceID SDL_OpenAudioDevice(const char *, int, const SDL_AudioSpec *, SDL_AudioSpec *, int);
static int observed_changes = 0;


/* A backend enlarges its quantum but preserves exact PCM format, rate and channel layout. */
extern "C" SDL_AudioDeviceID
fixture_native_open(const char *name, int capture, const SDL_AudioSpec *wanted, SDL_AudioSpec *obtained, int changes)
{
    observed_changes = changes;
    SDL_AudioSpec actual = *wanted;
    actual.samples = 8192;
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(name, capture, &actual, obtained, changes);
    /* Reproduce SDL's old behavior when the caller forbids publishing a changed quantum. */
    if (device && !(changes & SDL_AUDIO_ALLOW_SAMPLES_CHANGE))
    {
        obtained->samples = wanted->samples;
    }
    return device;
}
#endif


int main(int argc, char **argv)
{
#if defined(FVM_HAS_SDL_AUDIO)
    if (argc == 1)
    {
        return 0; /* Native access is an explicit opt-in test, not implied hardware acceptance. */
    }
    if (!(argc == 2 && !std::strcmp(argv[1], "--dummy")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(std::getenv("SDL_AUDIODRIVER") && !std::strcmp(std::getenv("SDL_AUDIODRIVER"), "dummy")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SDLAudioQueue *device = 0;
    FVM_AudioQueueBackend backend;
    if (!(fixture_queue_open(48000, 2, 1024, &device, &backend) == FVM_OK && device))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(observed_changes == SDL_AUDIO_ALLOW_SAMPLES_CHANGE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SDLAudioQueueInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_audio_sdl_queue_info(device, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(info.sample_rate == 48000 && info.channels == 2 && info.device_buffer_frames == 8192))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(backend.sample_rate == 48000 && backend.channels == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_sdl_queue_close(device) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    (void)argc;
    (void)argv;
#endif
    return 0;
}
