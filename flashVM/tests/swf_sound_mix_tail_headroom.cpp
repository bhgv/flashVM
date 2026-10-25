/* Adaptive native tails use partial FIFO room without splitting exact rejected retries. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_mix_gate_fixture.hpp"


int main()
{
    GateFixture f;
    size_t n = 91;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 0, 4, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 3, 4, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 0, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 1, &n) == FVM_OK && n == 1 && f.fifo.count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_OK && n == 2 && f.fifo.count == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t calls = f.fifo.calls;
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.mix, 2, 4, &n) == FVM_NOT_FOUND && !n && f.fifo.calls == calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_write = 2;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_NOT_FOUND && !n && f.fifo.count == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueuePending p;
    p.struct_size = sizeof(p);
    if (!(fvm_swf_sound_mix_queue_pending(f.mix, &p) == FVM_OK && p.frames == 1 &&
          p.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_write = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_OK && n == 1 && f.fifo.count == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 0;
    f.fifo.fail_write = 1;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_ERROR && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    calls = f.fifo.calls;
    f.fifo.fail_write = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 1, 4, &n) == FVM_NOT_FOUND && !n && f.fifo.calls == calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 1, &n) == FVM_NOT_FOUND && !n && f.fifo.calls == calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_OK && n == 2 && f.fifo.count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f.fifo.total == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < 6 * 2; ++i)
    {
        if (!(f.fifo.samples[i] == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_SWFSoundMixQueueClock c;
    c.struct_size = sizeof(c);
    if (!(fvm_swf_sound_mix_queue_clock_info(f.mix, &c) == FVM_OK && c.submitted_frames == 6 && c.played_frames == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
