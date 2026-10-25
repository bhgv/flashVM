/* Authored root Stop holds source phase while already-started events drain in both VM modes. */
#include <stdio.h>
#include <stdlib.h>
#include "mixed_native.hpp"
#include <cstdio>


static Bytes stopped_movie()
{
    Bytes tags, head, pcm, stop;
    Samples samples;
    for (unsigned i = 0; i < 8; ++i)
    {
        samples.push_back(2000);
    }
    definition(tags, 7, 3, 1, samples);
    head.push_back(0x0e);
    head.push_back(0x3e);
    sound_fixture::word(head, 4);
    sound_fixture::tag(tags, 45, head);
    for (unsigned i = 0; i < 4; ++i)
    {
        sound_fixture::word(pcm, 100);
    }
    sound_fixture::tag(tags, 19, pcm);
    event(tags, 7);
    stop.push_back(7);
    stop.push_back(0);
    sound_fixture::tag(tags, 12, stop);
    return sound_fixture::movie(tags);
}
#ifndef FVM_TEST_MIXED_STOP_FIXTURE_ONLY


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
        FVM_SWFStreamConverter initial = f.audio.stream_segments[0].converter;
        f.advance();
        if (!(f.audio.stream_movie_paused && f.audio.stream_frame_ready && f.audio.events == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int has = 0;
        if (!(fvm_desktop_audio_stream_pending(&f.audio, &has) == FVM_OK && has))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_audio_pause(&f.audio, 1) == FVM_OK &&
              fvm_desktop_audio_stream_pending(&f.audio, &has) == FVM_OK && !has))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_audio_pause(&f.audio, 0) == FVM_OK && fvm_gc_collect(f.context, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 8; ++i)
        {
            f.fifo.count = 0;
            f.refill();
        }
        if (!(f.fifo.total == 10 && !memcmp(&initial, &f.audio.stream_segments[0].converter, sizeof(initial))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 10; ++i)
        {
            if (!(f.fifo.samples[i * 2] == (i < 9 ? 2000 : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        f.fifo.count = 0;
        if (!(fvm_desktop_audio_stream_pending(&f.audio, &has) == FVM_OK && !has))
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
        if (!(f.fifo.total == 15))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 10; i < 15; ++i)
        {
            if (!(f.fifo.samples[i * 2] == 100))
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
    std::printf("PASS authored root stop: interpreter=1 native_jit=%u\n", executed > 1 ? 1U : 0U);
    return 0;
}

#endif
