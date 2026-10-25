/* Four real OS workers independently stop/restart around rejected mixed composition. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"
#include "../ports/thread_port.h"


struct StopJob
{
    short first;
    unsigned done;
};


static void worker(void *raw)
{
    StopJob *job = (StopJob *)raw;


    IdentityFixture f(true, job->first);
    for (unsigned epoch = 0; epoch < 64; ++epoch)
    {
        f.fifo.total = 0;
        FVM_SWFSoundVoice *v = f.clone(0);
        if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &v, (size_t)-1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t n = 0;
        f.fifo.fail = 1;
        if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_stop_all(f.queue) == FVM_OK && !active(f.mixer)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        v = f.clone(1);
        if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &v, (size_t)-1) == FVM_OK && !v))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail = 0;
        if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK &&
              fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 4; ++i)
        {
            if (!(f.fifo.saved[i] == job->first + (short)(i % 2)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_SWFSoundMixQueueIdentityInfo id;
        id.struct_size = sizeof(id);
        if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &id) == FVM_OK && id.end_frame == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_stop_all(f.queue) == FVM_OK && f.fifo.count == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(f.adapted.clear(f.adapted.userdata) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++job->done;
    }
}


int main()
{
    StopJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].first = (short)(100 + i * 100);
        jobs[i].done = 0;
        threads[i] = fvm_port_thread_start(worker, jobs + i);
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
