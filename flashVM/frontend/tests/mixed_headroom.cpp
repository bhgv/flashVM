/* Real C89 desktop scheduling fills one-frame room during authored Stop without advancing base. */
#define FVM_TEST_MIXED_STOP_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "mixed_authored_stop.cpp"


int main()
{
    unsigned executed = 0;
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        if (mode && !fvm_jit_available())
        {
            continue;
        }
        Bytes swf = stopped_movie();


        MixedNative f(mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, &swf);
        FVM_SWFStreamConverter held = f.audio.stream_segments[0].converter;
        f.advance();
        f.refill();
        if (!(f.audio.stream_movie_paused && f.fifo.total == 4 && f.fifo.count == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.count = 3;
        f.fifo.fail_write = 1;
        if (!(fvm_test_mixed_refill(&f.audio) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueuePending p;
        p.struct_size = sizeof(p);
        if (!(fvm_swf_sound_mix_queue_pending(f.audio.composition, &p) == FVM_OK && p.frames == 1 &&
              p.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(f.context, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_write = 0;
        f.refill();
        while (f.fifo.total < 9)
        {
            if (!(f.fifo.count == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            --f.fifo.count;
            f.refill();
        }
        if (!(f.fifo.total == 9 && !active(f.audio.mixer) &&
              !memcmp(&held, &f.audio.stream_segments[0].converter, sizeof(held))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 18; ++i)
        {
            if (!(f.fifo.samples[i] == 2000))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        f.fifo.count = 0;
        f.refill();
        if (!(f.fifo.total == 9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamOutputInfo info;
        info.struct_size = sizeof(info);
        if (!(fvm_desktop_audio_stream_info(&f.audio, &info) == FVM_OK && info.submitted_frames == 9 &&
              info.played_frames == 9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value out;
        if (!(fvm_object_call(f.context, f.clip, "play", 0, 0, &out) == FVM_OK &&
              fvm_desktop_audio_timeline(f.context, f.clip, &f.audio) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 4; ++i)
        {
            f.fifo.count = 0;
            f.refill();
        }
        if (!(f.fifo.total == 14))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 18; i < 28; ++i)
        {
            if (!(f.fifo.samples[i] == 100))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_ExecutionStats stats;
        stats.struct_size = sizeof(stats);
        if (!(fvm_execution_get_stats(f.context, &stats) == FVM_OK &&
              (mode ? stats.native_entries : stats.interpreter_entries) > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++executed;
    }
    std::printf("PASS partial desktop FIFO: interpreter=1 native_jit=%u\n", executed > 1 ? 1U : 0U);
    return 0;
}
