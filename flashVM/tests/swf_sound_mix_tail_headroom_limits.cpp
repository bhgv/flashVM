/* Seed exact clock boundaries; a one-frame request succeeds where a two-frame write cannot. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/swf/sound_mix_queue.cpp"
#include "swf_stream_mix_gate_fixture.hpp"


int main()
{
    GateFixture f;
    size_t n = 91;
    f.mix->clock_submitted = f.mix->clock_played = FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - 1;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 1, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f.mix->clock_submitted == FVM_SWF_SOUND_OUTPUT_MAX_CLOCK && f.fifo.total == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 0;
    size_t calls = f.fifo.calls;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 1, &n) == FVM_UNSUPPORTED && !n && f.fifo.calls == calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!f.mix->pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f.adapted.clear(f.adapted.userdata) == FVM_OK && !f.mix->clock_submitted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 1, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        GateFixture g;
        g.fifo.count = 1;
        if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(g.mix, 2, 1, &n) == FVM_ERROR && !n && !g.fifo.calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    {
        GateFixture g;
        if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(g.mix, 2, FVM_SWF_SOUND_OUTPUT_MAX_QUEUE + 1UL, &n) ==
                  FVM_INVALID_ARGUMENT &&
              !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
