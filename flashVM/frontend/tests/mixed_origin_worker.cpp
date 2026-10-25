/* Real native worker drains event voices under authored Stop, respects host pause and resumes base. */
#define FVM_TEST_MIXED_STOP_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "mixed_authored_stop.cpp"


static void wait_frames(FVM_DesktopAudio *a, double frames)
{
    FVM_SWFStreamMixInfo info;
    info.struct_size = sizeof(info);
    unsigned wait = 0;
    for (; wait < 400; ++wait)
    {
        if (!(fvm_desktop_audio_mix_info(a, &info) == FVM_OK && SDL_AtomicGet(&a->error) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(info.origins.base_submitted_frames + info.origins.tail_submitted_frames == info.stream.submitted_frames &&
              info.origins.base_played_frames + info.origins.tail_played_frames == info.stream.played_frames &&
              info.origins.base_queued_frames + info.origins.tail_queued_frames == info.stream.queued_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (info.stream.submitted_frames == frames && !info.stream.queued_frames)
        {
            break;
        }
        SDL_Delay(5);
    }
    if (!(wait < 400))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


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
        Bytes swf = stopped_movie();
        FVM_Object *clip = 0;
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
              fvm_movieclip_load_swf_ex(c, swf.begin(), swf.size(), "root", FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) ==
                  FVM_OK &&
              fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_DesktopAudio a;
        if (!(fvm_desktop_audio_open_mixed_policy(c, clip, &a, 1, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamConverter initial = a.stream_segments[0].converter;
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && fvm_desktop_audio_timeline(c, clip, &a) == FVM_OK &&
              a.stream_movie_paused))
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
        if (!(fvm_swf_sound_mix_queue_clock_info(a.composition, &clock) == FVM_OK && !clock.submitted_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        SDL_UnlockMutex(a.mutex);
        if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_desktop_audio_pause(&a, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        wait_frames(&a, 9);
        FVM_SWFSoundMixQueueOriginInfo origin;
        origin.struct_size = sizeof(origin);
        if (!(fvm_desktop_audio_origin_info(&a, &origin) == FVM_OK && !origin.base_submitted_frames &&
              !origin.base_played_frames && origin.tail_submitted_frames == 9 && origin.tail_played_frames == 9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_LockMutex(a.mutex) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!memcmp(&initial, &a.stream_segments[0].converter, sizeof(initial))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        SDL_UnlockMutex(a.mutex);
        int pending = 91;
        if (!(fvm_desktop_audio_stream_pending(&a, &pending) == FVM_OK && !pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value out;
        if (!(fvm_object_call(c, clip, "play", 0, 0, &out) == FVM_OK &&
              fvm_desktop_audio_timeline(c, clip, &a) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        wait_frames(&a, 14);
        if (!(fvm_desktop_audio_origin_info(&a, &origin) == FVM_OK && origin.base_submitted_frames == 5 &&
              origin.base_played_frames == 5 && origin.tail_submitted_frames == 9 && origin.tail_played_frames == 9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_audio_stop_all(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_audio_origin_info(&a, &origin) == FVM_OK && !origin.base_submitted_frames &&
              !origin.tail_submitted_frames && !origin.base_played_frames && !origin.tail_played_frames))
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
        fvm_desktop_audio_close(&a);
        fvm_gc_unpin(c, clip);
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
        ++executed;
    }
    std::printf("PASS real origin-accounted worker: modes=%u\n", executed);
    return 0;
}
