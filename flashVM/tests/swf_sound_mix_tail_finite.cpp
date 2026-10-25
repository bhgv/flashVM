/* Finite tails trim fresh final work, but preserve retries created by legacy padded rendering. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_mix_gate_fixture.hpp"


int main()
{
    {
        GateFixture f;
        size_t n = 0;
        if (!(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix, 1, 4, &n) == FVM_OK && n == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        while (f.fifo.total < 14)
        {
            f.fifo.count = 0;
            if (!(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix, 2, 4, &n) == FVM_OK && n && n < 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(f.fifo.total == 14 && n == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.count = 0;
        if (!(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix, 2, 4, &n) == FVM_NOT_FOUND && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 28; ++i)
        {
            if (!(f.fifo.samples[i] == (i < 18 ? 0 : -30000)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    {
        GateFixture f;
        size_t n = 0;
        for (unsigned i = 0; i < 13; ++i)
        {
            f.fifo.count = 0;
            if (!(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix, 1, 4, &n) == FVM_OK && n == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        f.fifo.count = 0;
        f.fifo.fail_write = 1;
        if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_ERROR && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_stop_all(f.mix) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_write = 0;
        if (!(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix, 2, 4, &n) == FVM_OK && n == 2 && f.fifo.total == 15))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(f.fifo.samples[26] == -30000 && f.fifo.samples[28] == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
