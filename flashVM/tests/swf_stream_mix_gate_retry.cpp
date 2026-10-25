/* Tail retries precede newly authorized base; held base retries block event-only substitution. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_mix_gate_fixture.hpp"


int main(int argc, char **argv)
{
    bool exceptions = argc == 1;
    if (!(argc == 1 || (argc == 2 && !strcmp(argv[1], "--no-throw"))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        GateFixture f;
        f.fifo.reenter = 1;
        FVM_SWFStreamConverter cursor = f.converter;
        size_t n = 91;
        f.fifo.fail_count = 1;
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_ERROR && !n &&
              !memcmp(&cursor, &f.converter, sizeof(cursor))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_count = 0;
        f.fifo.fail_write = 1;
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_ERROR && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (exceptions)
        {
            f.fifo.throw_write = 1;
            if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n) == FVM_ERROR && !n))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            f.fifo.throw_write = 0;
        }
        f.fifo.fail_write = 0;
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n) == FVM_OK && n == 2 &&
              !memcmp(&cursor, &f.converter, sizeof(cursor))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n) == FVM_OK && n == 2 &&
              f.fifo.samples[4] == 30000 && f.fifo.samples[6] == -25125))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    {
        GateFixture f;
        size_t n = 0;
        f.fifo.fail_write = 2;
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n) == FVM_NOT_FOUND && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueuePending pending;
        pending.struct_size = sizeof(pending);
        if (!(fvm_swf_sound_mix_queue_pending(f.mix, &pending) == FVM_OK &&
              pending.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE && pending.frames == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamConverter cursor = f.converter;
        size_t calls = f.fifo.calls;
        f.fifo.fail_write = 0;
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_NOT_FOUND && !n &&
              f.fifo.calls == calls && !memcmp(&cursor, &f.converter, sizeof(cursor))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n) == FVM_OK && n == 2 &&
              !memcmp(&cursor, &f.converter, sizeof(cursor))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(f.fifo.samples[0] == 30000 && f.fifo.samples[2] == -25125))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
