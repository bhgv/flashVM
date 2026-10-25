/* Opt-in paired finite scheduling retains TAIL priority through callback failure and recursion. */
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
    GateFixture f;
    f.fifo.reenter = 1;
    FVM_SWFStreamConverter held = f.converter;
    size_t n = 91;
    f.fifo.fail_count = 1;
    if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 0, &n) == FVM_ERROR && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_count = 0;
    f.fifo.fail_write = 2;
    if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 0, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (exceptions)
    {
        f.fifo.throw_write = 1;
        if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 1, &n) == FVM_ERROR && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.throw_write = 0;
    }
    if (!(fvm_swf_sound_mix_queue_stop_all(f.mix) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_write = 0;
    if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 1, &n) == FVM_OK && n == 2 &&
          !memcmp(&held, &f.converter, sizeof(held))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_clear = 1;
    if (!(fvm_swf_stream_output_seek(&f.output, 0, 0) == FVM_ERROR && f.output.info.submitted_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_clear = 0;
    if (!(fvm_swf_stream_output_seek(&f.output, 0, 0) == FVM_OK && !f.output.info.submitted_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
