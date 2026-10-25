/* Event-only work at gate zero leaves base untouched; opening the gate resumes base and both clocks. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_mix_gate_fixture.hpp"


int main()
{
    GateFixture f;
    FVM_SWFStreamConverter before = f.converter;
    size_t n = 91;
    for (unsigned i = 0; i < 7; ++i)
    {
        f.fifo.count = 0;
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_OK && n == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!memcmp(&before, &f.converter, sizeof(before)) && f.fifo.total == 14))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 14; ++i)
    {
        if (!(f.fifo.samples[i * 2] == (i < 9 ? 0 : -30000) && f.fifo.samples[i * 2 + 1] == f.fifo.samples[i * 2]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    f.fifo.count = 0;
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 8; ++i)
    {
        f.fifo.count = 0;
        FVM_Result r = fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n);
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
    const short expected[] = {30000, -25125, 11875, -10250, 250, -1875, 10500, 4125, -30000};
    if (!(f.fifo.total == 23))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 9; ++i)
    {
        if (!(f.fifo.samples[(i + 14) * 2] == expected[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_SWFStreamOutputInfo info;
    info.struct_size = sizeof(info);
    f.fifo.count = 0;
    if (!(fvm_swf_stream_output_info(&f.output, &info) == FVM_OK && info.eof && info.submitted_frames == 23 &&
          info.played_frames == 23))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueClock clock;
    clock.struct_size = sizeof(clock);
    if (!(fvm_swf_sound_mix_queue_clock_info(f.mix, &clock) == FVM_OK && clock.submitted_frames == 23 &&
          clock.played_frames == 23))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
