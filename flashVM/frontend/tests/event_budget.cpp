/* Actual owner callback applies aggregate event storage and preserves native producer access. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/audio.h"
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"

using namespace conversion_fixture;


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Samples small, large;
    small.resize(12000);
    large.resize(40000);
    for (size_t i = 0; i < small.size(); ++i)
    {
        small[i] = 1234;
    }
    Bytes tags;
    definition(tags, 7, 3, 1, small);
    definition(tags, 8, 3, 1, large);
    event(tags, 7);
    event(tags, 7, FVM_SWF_SOUND_EVENT_NO_MULTIPLE);
    event(tags, 8);
    Bytes swf = sound_fixture::movie(tags);
    FVM_Object *clip = 0;
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
          fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_DesktopAudio audio;
    if (!(fvm_desktop_audio_open_paused(c, clip, &audio, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* One owner frame runs all three real callbacks: the oversized third owner fails. */
    if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OUT_OF_MEMORY && audio.events == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_LockMutex(audio.mutex) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixerStorageInfo storage;
    storage.struct_size = sizeof(storage);
    if (!(fvm_swf_sound_mixer_storage_info(audio.mixer, &storage) == FVM_OK &&
          storage.retained_bytes <= FVM_DESKTOP_AUDIO_EVENT_BYTES))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixerInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mixer_info(audio.mixer, &info) == FVM_OK && info.active_voices == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_UnlockMutex(audio.mutex);
    if (!(fvm_desktop_audio_pause(&audio, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_Delay(80);
    FVM_SWFSoundOutputInfo output;
    output.struct_size = sizeof(output);
    if (!(fvm_desktop_audio_info(&audio, &output) == FVM_OK && output.submitted_frames > 0 &&
          SDL_AtomicGet(&audio.error) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_audio_stop_all(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_LockMutex(audio.mutex) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_storage_info(audio.mixer, &storage) == FVM_OK && !storage.voice_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_UnlockMutex(audio.mutex);
    if (!(fvm_movieclip_set_sound_callback(c, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_audio_close(&audio);
    fvm_gc_unpin(c, clip);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
