/* Four real OS workers own distinct ID deadlines, queued PCM and reusable native templates. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"
#include "../ports/thread_port.h"


struct IdentityJob
{
    short first;
    unsigned done;
};


static void identity_worker(void *raw)
{
    IdentityJob *job = (IdentityJob *)raw;


    IdentityFixture f(true, job->first);
    for (unsigned iteration = 0; iteration < 64; ++iteration)
    {
        f.fifo.total = 0;
        FVM_SWFSoundVoice *voice = f.clone(0), *duplicate = f.clone(1);
        if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK && !voice))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t n = 0;
        f.fifo.fail = 1;
        if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_ERROR && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &duplicate, 0) == FVM_OK && !duplicate))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail = 0;
        if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK &&
              fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK &&
              fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &n) == FVM_OK && !active(f.mixer)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int yes = 0;
        if (!(fvm_swf_sound_mix_queue_sound_active(f.queue, 7, &yes) == FVM_OK && yes))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 5; ++i)
        {
            if (!(f.fifo.saved[i] == (short)(job->first + i)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        f.fifo.count = 0;
        FVM_SWFSoundMixQueueIdentityInfo info;
        info.struct_size = sizeof(info);
        if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && !info.active &&
              info.played_frames == 5 && info.end_frame == 5))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(f.adapted.clear(f.adapted.userdata) == FVM_OK &&
              fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && !info.end_frame &&
              !info.played_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++job->done;
    }
}


int main()
{
    IdentityJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].first = (short)(100 + i * 100);
        jobs[i].done = 0;
        threads[i] = fvm_port_thread_start(identity_worker, jobs + i);
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
