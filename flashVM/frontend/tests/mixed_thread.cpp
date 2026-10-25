/* A real SDL producer cannot run the current stream before owner-side event delivery completes. */
#define FVM_DESKTOP_MIXED_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "mixed_open.cpp"
#include "flashvm/execution_mode.h"
#include <cstdio>


int main()
{
    unsigned executed = 0;
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
              fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes swf = mixed_movie();
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *clip = 0;
        if (!(fvm_movieclip_load_swf_ex(c, swf.begin(), swf.size(), "root", FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) ==
                  FVM_OK &&
              fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_DesktopAudio a;
        if (!(fvm_desktop_audio_open(c, clip, &a) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && a.events == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        SDL_Delay(25);
        if (!(SDL_LockMutex(a.mutex) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueueClock clock;
        clock.struct_size = sizeof(clock);
        if (!(fvm_swf_sound_mix_queue_clock_info(a.composition, &clock) == FVM_OK && !clock.submitted_frames &&
              !a.stream_frame))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        SDL_UnlockMutex(a.mutex);
        if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_desktop_audio_timeline(c, clip, &a) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamOutputInfo info;
        info.struct_size = sizeof(info);
        unsigned wait = 0;
        for (; wait < 400; ++wait)
        {
            if (!(fvm_desktop_audio_stream_info(&a, &info) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (info.submitted_frames >= 1029 && !info.queued_frames)
            {
                break;
            }
            if (!(SDL_AtomicGet(&a.error) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            SDL_Delay(5);
        }
        if (!(wait < 400 && info.submitted_frames == 1029 && info.played_frames == 1029))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && a.events == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        SDL_Delay(25);
        if (!(SDL_LockMutex(a.mutex) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_clock_info(a.composition, &clock) == FVM_OK && clock.submitted_frames == 1029 &&
              !a.stream_frame))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        SDL_UnlockMutex(a.mutex);
        if (!(fvm_desktop_audio_timeline(c, clip, &a) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (wait = 0; wait < 400; ++wait)
        {
            if (!(fvm_desktop_audio_stream_info(&a, &info) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (info.submitted_frames >= 2053 && !info.queued_frames)
            {
                break;
            }
            if (!(SDL_AtomicGet(&a.error) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            SDL_Delay(5);
        }
        if (!(wait < 400 && info.submitted_frames == 2053))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_set_sound_callback(c, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_desktop_audio_close(&a);
        fvm_gc_unpin(c, clip);
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
        ++executed;
    }
    std::printf("PASS real SDL mixed worker authorization: modes=%u\n", executed);
    return 0;
}
