/* Partial tail cancellation, callback recursion and exceptions preserve atomic retry ownership. */
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
    size_t n = 91;
    f.fifo.reenter = 2;
    f.fifo.fail_count = 1;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 1, &n) == FVM_ERROR && !n && !f.fifo.calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_count = 0;
    f.fifo.fail_write = 2;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 1, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (exceptions)
    {
        f.fifo.throw_write = 1;
        if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_ERROR && !n))
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
    FVM_SWFSoundMixQueuePending p;
    p.struct_size = sizeof(p);
    if (!(fvm_swf_sound_mix_queue_pending(f.mix, &p) == FVM_OK && p.frames == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_clear = 1;
    if (!(f.adapted.clear(f.adapted.userdata) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pending(f.mix, &p) == FVM_OK && p.frames == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_write = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix, 2, 4, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_clear = 0;
    if (!(f.adapted.clear(f.adapted.userdata) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueClock clock;
    clock.struct_size = sizeof(clock);
    if (!(fvm_swf_sound_mix_queue_clock_info(f.mix, &clock) == FVM_OK && !clock.submitted_frames &&
          !clock.played_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        GateFixture b;
        short base[4] = {100, 200, 300, 400};
        b.fifo.fail_write = 1;
        if (!(b.adapted.write(b.adapted.userdata, base, 2) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t calls = b.fifo.calls;
        if (!(fvm_swf_sound_mix_queue_pump_tail_up_to(b.mix, 2, 4, &n) == FVM_INVALID_ARGUMENT && !n &&
              b.fifo.calls == calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
