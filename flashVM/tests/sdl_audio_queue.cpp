/* Default stub checks plus an explicitly selected real SDL2 dummy-driver protocol run. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound_output.h"
#include <cstring>
#include <cstdlib>

#if defined(FVM_HAS_SDL_AUDIO)
#define SDL_MAIN_HANDLED
#include <SDL.h>

#if SDL_VERSION_ATLEAST(2, 0, 7)
static bool native_fail = false;


static void *SDLCALL native_malloc(size_t size)
{
    return native_fail ? 0 : std::malloc(size);
}


static void *SDLCALL native_calloc(size_t count, size_t size)
{
    return native_fail ? 0 : std::calloc(count, size);
}


static void *SDLCALL native_realloc(void *p, size_t size)
{
    return native_fail ? 0 : std::realloc(p, size);
}


static void SDLCALL native_free(void *p)
{
    std::free(p);
}
#endif
#endif


int main(int argc, char **argv)
{
    FVM_SDLAudioQueue *device = (FVM_SDLAudioQueue *)1;
    FVM_AudioQueueBackend backend;
    std::memset(&backend, 0x7e, sizeof(backend));
    if (!(fvm_audio_sdl_queue_open(0, 2, 512, &device, &backend) == FVM_INVALID_ARGUMENT && !device && !backend.write))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_sdl_queue_open(48000, 3, 512, &device, &backend) == FVM_INVALID_ARGUMENT && !device))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_sdl_queue_open(48000, 2, 65, &device, &backend) == FVM_INVALID_ARGUMENT && !device))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_sdl_queue_close(0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if defined(FVM_HAS_SDL_AUDIO)
    if (argc == 1)
    {
        return 0; /* Device access is a separately named explicit test, never implied native
                     acceptance. */
    }
    if (!(argc == 2 && !std::strcmp(argv[1], "--dummy")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *driver = std::getenv("SDL_AUDIODRIVER");
    if (!(driver && !std::strcmp(driver, "dummy")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if SDL_VERSION_ATLEAST(2, 0, 7)
    if (!(SDL_SetMemoryFunctions(native_malloc, native_calloc, native_realloc, native_free) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    if (!(fvm_audio_sdl_queue_open(48000, 2, 64, &device, &backend) == FVM_OK && device))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SDLAudioQueueInfo native_info;
    std::memset(&native_info, 0x5a, sizeof(native_info));
    native_info.struct_size = 0;
    FVM_SDLAudioQueueInfo preserved = native_info;
    if (!(fvm_audio_sdl_queue_info(device, &native_info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&native_info, &preserved, sizeof(native_info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    native_info.struct_size = sizeof(native_info);
    if (!(fvm_audio_sdl_queue_info(device, &native_info) == FVM_OK && native_info.sample_rate == 48000 &&
          native_info.channels == 2 && native_info.device_buffer_frames > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_GetCurrentAudioDriver() && !std::strcmp(SDL_GetCurrentAudioDriver(), "dummy")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixerConfig config = {sizeof(config), 48000, 2, 4, FVM_SWF_SOUND_MIXER_CONVERT};
    FVM_SWFSoundMixer *mixer = 0;
    if (!(fvm_swf_sound_mixer_create_configured(&config, &mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundOutput *output = 0;
    if (!(fvm_swf_sound_output_create(mixer, &backend, 128, 512, &output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t accepted = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 128))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_swf_sound_output_pump(output, &accepted) == FVM_NOT_FOUND && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundOutputInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_output_info(output, &info) == FVM_OK && info.queued_frames == 512 && !info.played_frames &&
          info.paused))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_Delay(20);
    if (!(fvm_swf_sound_output_info(output, &info) == FVM_OK && info.queued_frames == 512 && !info.played_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pause(output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Uint32 start = SDL_GetTicks();
    do
    {
        SDL_Delay(1);
        if (!(fvm_swf_sound_output_info(output, &info) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    } while (info.played_frames < 512 && SDL_GetTicks() - start < 2000);
    if (!(info.played_frames == 512 && !info.queued_frames && info.starved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 128))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_info(output, &info) == FVM_OK && info.submitted_frames == 640 &&
          info.played_frames >= 512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_info(output, &info) == FVM_OK && info.paused && !info.queued_frames &&
          !info.submitted_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if SDL_VERSION_ATLEAST(2, 0, 7)
    /* A paused device cannot consume the injected write. Its queue pool is much smaller than this block. */
    short *large = new short[FVM_SWF_SOUND_OUTPUT_MAX_BLOCK * 2];
    std::memset(large, 0, FVM_SWF_SOUND_OUTPUT_MAX_BLOCK * 2 * sizeof(short));
    native_fail = true;
    FVM_Result failed = backend.write(backend.userdata, large, FVM_SWF_SOUND_OUTPUT_MAX_BLOCK);
    native_fail = false;
    if (!(failed == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t untouched = 777;
    if (!(backend.queued(backend.userdata, &untouched) == FVM_ERROR && untouched == 777))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pause(output, 0) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 128))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    delete[] large;
#endif
    fvm_swf_sound_output_close(output);
    fvm_swf_sound_mixer_close(mixer);
    if (!(fvm_audio_sdl_queue_close(device) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    (void)argc;
    (void)argv;
    if (!(fvm_audio_sdl_queue_open(48000, 2, 512, &device, &backend) == FVM_UNSUPPORTED && !device && !backend.write &&
          !backend.queued))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
