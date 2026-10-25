/* A slow owner-side native allocation must not hold the actual SDL PCM producer lock. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/audio.h"
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/execution_mode.h"
#include <cstdlib>

using namespace conversion_fixture;
static FVM_DesktopAudio *observed = 0;
static size_t observed_progress = 0;
static bool armed = false;


extern "C" void fvm_test_event_prepare_delay(void)
{
    if (!armed)
    {
        return;
    }
    armed = false;
    FVM_SWFSoundOutputInfo before, after;
    before.struct_size = sizeof(before);
    after.struct_size = sizeof(after);
    if (!(fvm_desktop_audio_info(observed, &before) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_Delay(350); /* More than three complete default queue reserves. */
    if (!(fvm_desktop_audio_info(observed, &after) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    observed_progress = (size_t)(after.submitted_frames - before.submitted_frames);
}


int main()
{
    for (unsigned jit = 0; jit < 2; ++jit)
    {
        FVM_Config cfg;
        FVM_Runtime *rt = 0;
        FVM_Context *c = 0;
        if (!(fvm_config_init(&cfg) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK && fvm_context_create(rt, &c) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (jit && !fvm_jit_available())
        {
            fvm_context_destroy(c);
            fvm_runtime_destroy(rt);
            continue;
        }
        if (!(fvm_execution_set_mode(c, jit ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Samples samples;
        samples.resize(88200);
        for (size_t i = 0; i < samples.size(); ++i)
        {
            samples[i] = 2000;
        }
        Bytes tags, empty, action;
        const unsigned char raw[] = {0x96, 5, 0, 7, 1, 0, 0, 0, 0x17, 0};
        for (size_t i = 0; i < sizeof(raw); ++i)
        {
            action.push_back(raw[i]);
        }
        definition(tags, 7, 3, 1, samples);
        sound_fixture::tag(tags, 12, action);
        event(tags, 7);
        sound_fixture::tag(tags, 1, empty);
        sound_fixture::tag(tags, 12, action);
        event(tags, 7);
        Bytes swf = sound_fixture::movie(tags);
        swf[12] = 2;
        FVM_Object *clip = 0;
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
              fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &clip) == FVM_OK &&
              fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_DesktopAudio audio;
        if (!(fvm_desktop_audio_open(c, clip, &audio) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && audio.events == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        SDL_Delay(80);
        observed = &audio;
        observed_progress = 0;
        armed = true;
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && !armed && audio.events == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_ExecutionStats stats;
        stats.struct_size = sizeof(stats);
        if (!(fvm_execution_get_stats(c, &stats) == FVM_OK &&
              (jit ? stats.native_entries : stats.interpreter_entries) > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(observed_progress > 10000))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        } /* Locked decode would advance zero submitted frames. */
        if (!(SDL_AtomicGet(&audio.error) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
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
        observed = 0;
        fvm_gc_unpin(c, clip);
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
    }
    return 0;
}
