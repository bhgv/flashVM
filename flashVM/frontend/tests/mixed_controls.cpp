/* Real root NoMultiple/StopSounds actions share mixed queue ownership through GC and native JIT. */
#include <stdio.h>
#include <stdlib.h>
#include "mixed_native.hpp"
#include <cstdio>


static void mixed_action(Bytes &tags, unsigned visit, bool stop = false)
{
    Bytes raw;
    raw.push_back(0x96);
    sound_fixture::word(raw, 11);
    raw.push_back(0);
    const char name[] = "seen";
    for (unsigned i = 0; i < sizeof(name); ++i)
    {
        raw.push_back((unsigned char)name[i]);
    }
    raw.push_back(7);
    sound_fixture::integer(raw, visit);
    raw.push_back(0x1d);
    if (stop)
    {
        raw.push_back(9);
    }
    raw.push_back(0);
    sound_fixture::tag(tags, 12, raw);
}


static Bytes controls_movie()
{
    Bytes tags, head, pcm, empty;
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
    mixed_action(tags, 1);
    sound_fixture::tag(tags, 1, empty);
    event(tags, 7, FVM_SWF_SOUND_EVENT_NO_MULTIPLE);
    mixed_action(tags, 2);
    sound_fixture::tag(tags, 1, empty);
    mixed_action(tags, 3, true);
    sound_fixture::tag(tags, 1, empty);
    event(tags, 7, FVM_SWF_SOUND_EVENT_NO_MULTIPLE);
    mixed_action(tags, 4);
    Bytes swf = sound_fixture::movie(tags);
    swf[12] = 4;
    return swf;
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
        Bytes swf = controls_movie();


        MixedNative f(mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, &swf);
        f.advance();
        for (unsigned i = 0; i < 3; ++i)
        {
            f.fifo.count = 0;
            f.refill();
        }
        if (!(f.fifo.total == 10 && !active(f.audio.mixer)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int yes = 0;
        if (!(fvm_swf_sound_mix_queue_sound_active(f.audio.composition, 7, &yes) == FVM_OK && yes))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.advance();
        if (!(f.audio.events == 2 && !active(f.audio.mixer)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(f.context, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_clear = 1;
        if (!(fvm_audio_stop_all(f.context) == FVM_ERROR && !f.audio.stops && f.fifo.count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_clear = 0;
        f.advance();
        if (!(f.audio.stops == 1 && f.audio.stream_cancelled && !f.fifo.count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueueClock clock;
        clock.struct_size = sizeof(clock);
        if (!(fvm_swf_sound_mix_queue_clock_info(f.audio.composition, &clock) == FVM_OK && !clock.submitted_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.advance();
        if (!(f.audio.events == 3 && active(f.audio.mixer) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t start = f.fifo.total;
        int pending = 0;
        if (!(fvm_desktop_audio_stream_pending(&f.audio, &pending) == FVM_OK && pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 8; ++i)
        {
            f.fifo.count = 0;
            f.refill();
        }
        if (!(f.fifo.total == start + 10))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < 10; ++i)
        {
            if (!(f.fifo.samples[(start + i) * 2] == (i < 9 ? 2000 : 0)))
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
    std::printf("PASS desktop mixed root controls: interpreter=1 native_jit=%u\n", executed > 1 ? 1U : 0U);
    return 0;
}
