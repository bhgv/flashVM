/* Actual SDL queue holds NoMultiple after short native retirement, across GC and owner controls. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/audio.h"
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include "flashvm/execution_mode.h"

using namespace conversion_fixture;


static void action(Bytes &tags, bool stop = false)
{
    const unsigned char raw[] = {0x96, 5, 0, 7, 1, 0, 0, 0, 0x17};
    Bytes data;
    for (size_t i = 0; i < sizeof(raw); ++i)
    {
        data.push_back(raw[i]);
    }
    if (stop)
    {
        data.push_back(9);
    }
    data.push_back(0);
    sound_fixture::tag(tags, 12, data);
}


static unsigned voices(FVM_DesktopAudio *audio)
{
    if (!(SDL_LockMutex(audio->mutex) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixerInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mixer_info(audio->mixer, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_UnlockMutex(audio->mutex);
    return info.active_voices;
}


static int playing(FVM_DesktopAudio *audio)
{
    int value = -1;
    if (!(SDL_LockMutex(audio->mutex) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_sound_active(audio->output, 7, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_UnlockMutex(audio->mutex);
    return value;
}


int main()
{
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        if (mode && !fvm_jit_available())
        {
            continue;
        }
        FVM_Config cfg;
        FVM_Runtime *rt = 0;
        FVM_Context *c = 0;
        if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
              fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Samples samples;
        for (unsigned i = 0; i < 7; ++i)
        {
            samples.push_back(2000);
        }
        Bytes tags, empty;
        definition(tags, 7, 3, 1, samples);
        event(tags, 7);
        action(tags);
        sound_fixture::tag(tags, 1, empty);
        event(tags, 7, FVM_SWF_SOUND_EVENT_NO_MULTIPLE);
        action(tags);
        sound_fixture::tag(tags, 1, empty);
        event(tags, 7, FVM_SWF_SOUND_EVENT_STOP);
        event(tags, 7, FVM_SWF_SOUND_EVENT_NO_MULTIPLE);
        action(tags);
        sound_fixture::tag(tags, 1, empty);
        action(tags, true);
        sound_fixture::tag(tags, 1, empty);
        event(tags, 7, FVM_SWF_SOUND_EVENT_NO_MULTIPLE);
        action(tags);
        Bytes swf = sound_fixture::movie(tags);
        swf[12] = 5;
        FVM_Object *clip = 0;
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
              fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &clip) == FVM_OK &&
              fvm_gc_pin(c, clip) == FVM_OK))
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
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && audio.events == 1 && voices(&audio) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_LockMutex(audio.mutex) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t n = 0;
        if (!(fvm_swf_sound_output_pump(audio.output, &n) == FVM_OK && n == 1024))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        SDL_UnlockMutex(audio.mutex);
        if (!(!voices(&audio) && playing(&audio)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && audio.events == 2 && !voices(&audio) && playing(&audio)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && audio.events == 4 && voices(&audio) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundOutputInfo info;
        info.struct_size = sizeof(info);
        if (!(fvm_desktop_audio_info(&audio, &info) == FVM_OK && info.queued_frames == 1024))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && audio.stops == 1 && !voices(&audio) && !playing(&audio)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && audio.events == 5 && voices(&audio) == 1 &&
              playing(&audio)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_audio_pause(&audio, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned wait = 0; wait < 200 && playing(&audio); ++wait)
        {
            SDL_Delay(5);
        }
        if (!(!voices(&audio) && !playing(&audio) && SDL_AtomicGet(&audio.error) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_ExecutionStats stats;
        stats.struct_size = sizeof(stats);
        if (!(fvm_execution_get_stats(c, &stats) == FVM_OK &&
              (mode ? stats.native_entries : stats.interpreter_entries) > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_set_sound_callback(c, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_desktop_audio_close(&audio);
        fvm_gc_unpin(c, clip);
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
    }
    return 0;
}
