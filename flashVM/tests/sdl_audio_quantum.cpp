/* Exercise production open with a real SDL dummy device behind a negotiating SDK shim. */
#include <cassert>
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
extern "C" SDL_AudioDeviceID fixture_native_open(const char *name, int capture,
    const SDL_AudioSpec *wanted, SDL_AudioSpec *obtained, int changes)
{
    observed_changes = changes;
    SDL_AudioSpec actual = *wanted;
    actual.samples = 8192;
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(name, capture, &actual, obtained, changes);
    /* Reproduce SDL's old behavior when the caller forbids publishing a changed quantum. */
    if (device && !(changes & SDL_AUDIO_ALLOW_SAMPLES_CHANGE))
        obtained->samples = wanted->samples;
    return device;
}
#endif
int main(int argc, char **argv)
{
#if defined(FVM_HAS_SDL_AUDIO)
    if (argc == 1)
        return 0; /* Native access is an explicit opt-in test, not implied hardware acceptance. */
    assert(argc == 2 && !std::strcmp(argv[1], "--dummy"));
    assert(std::getenv("SDL_AUDIODRIVER") && !std::strcmp(std::getenv("SDL_AUDIODRIVER"), "dummy"));
    FVM_SDLAudioQueue *device = 0;
    FVM_AudioQueueBackend backend;
    assert(fixture_queue_open(48000, 2, 1024, &device, &backend) == FVM_OK && device);
    assert(observed_changes == SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    FVM_SDLAudioQueueInfo info;
    info.struct_size = sizeof(info);
    assert(fvm_audio_sdl_queue_info(device, &info) == FVM_OK);
    assert(info.sample_rate == 48000 && info.channels == 2 && info.device_buffer_frames == 8192);
    assert(backend.sample_rate == 48000 && backend.channels == 2);
    assert(fvm_audio_sdl_queue_close(device) == FVM_OK);
#else
    (void)argc; (void)argv;
#endif
    return 0;
}
