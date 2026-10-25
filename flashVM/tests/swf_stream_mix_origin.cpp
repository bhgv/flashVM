/* Held source phase, real converted BASE and interleaved event TAIL produce distinct dequeue clocks. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_origin_fixture.hpp"


int main()
{
    GateFixture f;
    enable_origins(f);
    FVM_SWFStreamConverter held = f.converter;
    size_t n = 0;
    if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 0, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(&held, &f.converter, sizeof(held))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 1, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 0;
    FVM_SWFSoundMixQueueOriginInfo info = origins(f);
    if (!(info.base_played_frames == 2 && info.tail_played_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 20; ++i)
    {
        f.fifo.count = 0;
        FVM_Result r = fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 1, &n);
        if (r == FVM_NOT_FOUND)
        {
            break;
        }
        if (!(r == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    f.fifo.count = 0;
    info = origins(f);
    if (!(info.base_submitted_frames == 9 && info.base_played_frames == 9 && info.tail_submitted_frames == 5 &&
          info.tail_played_frames == 5 && f.fifo.total == 14))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        GateFixture g;
        enable_origins(g);
        if (!(fvm_swf_stream_output_pump_mix_finite_until(&g.output, g.mix, 0, &n) == FVM_OK && n == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_output_pump_mix_finite_until(&g.output, g.mix, 0, &n) == FVM_OK && n == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        g.fifo.drain_per_count = 1;
        if (!(fvm_swf_stream_output_pump_mix_finite_until(&g.output, g.mix, 1, &n) == FVM_OK && n == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(g.output.info.submitted_frames == 6 && g.output.info.played_frames == 3 &&
              g.output.info.queued_frames == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        g.fifo.drain_per_count = 0;
        info = origins(g);
        if (!(info.tail_played_frames == 3 && !info.base_played_frames && info.base_queued_frames == 2 &&
              info.tail_queued_frames == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
