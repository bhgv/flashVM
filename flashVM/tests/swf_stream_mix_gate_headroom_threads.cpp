/* Four OS workers independently consume one FIFO frame per short-retry native scheduling step. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_mix_gate_fixture.hpp"
#include "../ports/thread_port.h"


struct HeadroomJob
{
    unsigned done;
};


static void headroom_worker(void *raw)
{
    HeadroomJob *job = (HeadroomJob *)raw;
    for (unsigned epoch = 0; epoch < 64; ++epoch)
    {
        GateFixture f;
        FVM_SWFStreamConverter held = f.converter;
        size_t n = 0;
        for (unsigned i = 0; i < 2; ++i)
        {
            if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_OK && n == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        while (f.fifo.total < 14)
        {
            if (!(f.fifo.count == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            --f.fifo.count;
            f.fifo.fail_write = 2;
            if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_NOT_FOUND && !n))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            f.fifo.fail_write = 0;
            if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n) == FVM_OK && n == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!memcmp(&held, &f.converter, sizeof(held))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        for (unsigned frame = 0; frame < 14; ++frame)
        {
            for (unsigned channel = 0; channel < 2; ++channel)
            {
                if (!(f.fifo.samples[frame * 2 + channel] == (frame < 9 ? 0 : -30000)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        f.fifo.count = 0;
        if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_NOT_FOUND && !n &&
              f.fifo.total == 14))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueueIdentityInfo id;
        id.struct_size = sizeof(id);
        if (!(fvm_swf_sound_mix_queue_identity_info(f.mix, 8, &id) == FVM_OK && !id.active && id.played_frames == 14))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_output_seek(&f.output, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++job->done;
    }
}


int main()
{
    HeadroomJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].done = 0;
        threads[i] = fvm_port_thread_start(headroom_worker, jobs + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
        if (!(jobs[i].done == 64))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
