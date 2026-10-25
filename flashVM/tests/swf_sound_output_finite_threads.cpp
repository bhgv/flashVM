/* Four independent native owners verify exact finite retries, PCM and drained IDs after source teardown. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_output_finite_fixture.hpp"
#include "../ports/thread_port.h"


struct OutputJob
{
    unsigned done;
};


static void finite_output_worker(void *raw)
{
    OutputJob *job = (OutputJob *)raw;
    conversion_fixture::Samples samples;
    for (unsigned i = 0; i < 8; ++i)
    {
        samples.push_back((short)(2000 + i * 17));
    }
    for (unsigned epoch = 0; epoch < 64; ++epoch)
    {


        FiniteOutput f(8, true);
        f.start(0, true);
        size_t n = 0;
        for (unsigned step = 0; step < 3; ++step)
        {
            f.device.queued = 0;
            f.device.failure = 1;
            if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_ERROR && !n))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            f.device.failure = 0;
            if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_OK && n == (step < 2 ? 4 : 1)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(f.device.values == 18))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned frame = 0; frame < 9; ++frame)
        {
            for (unsigned ch = 0; ch < 2; ++ch)
            {
                if (!(f.device.history[frame * 2 + ch] ==
                      conversion_fixture::oracle(samples, 44100, 1, 48000, 2, frame, ch)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        f.device.queued = 0;
        int pending = 91;
        if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && !pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundOutputIdentityInfo id;
        id.struct_size = sizeof(id);
        if (!(fvm_swf_sound_output_identity_info(f.output, 7, &id) == FVM_OK && !id.active && id.end_frame == 9 &&
              id.played_frames == 9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++job->done;
    }
}


int main()
{
    OutputJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].done = 0;
        threads[i] = fvm_port_thread_start(finite_output_worker, jobs + i);
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
