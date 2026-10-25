/* Stop-all preserves each retry owner and admits a new identity beyond retained mixed bytes. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"


int main()
{
    for (unsigned tail = 0; tail < 2; ++tail)
    {
        IdentityFixture f;
        FVM_SWFSoundVoice *v = f.clone(0);
        if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &v, (size_t)-1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        short base[2] = {10, 20};
        size_t n = 0;
        f.fifo.fail = 1;
        if (!((tail ? fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n)
                    : f.adapted.write(f.adapted.userdata, base, 2)) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_stop_all(f.queue) == FVM_OK && !active(f.mixer)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueuePending pending;
        pending.struct_size = sizeof(pending);
        if (!(fvm_swf_sound_mix_queue_pending(f.queue, &pending) == FVM_OK && pending.frames == 2 &&
              pending.origin == (tail ? FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL : FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE)))
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
        FVM_SWFSoundMixQueueIdentityInfo id;
        id.struct_size = sizeof(id);
        if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &id) == FVM_OK && id.end_frame == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail = 0;
        if (!((tail ? fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n)
                    : f.adapted.write(f.adapted.userdata, base, 2)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const short expected[] = {100, 101, 100, 101};
        for (unsigned i = 0; i < 4; ++i)
        {
            if (!(f.fifo.saved[i] == expected[i] + (!tail && i < 2 ? base[i] : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &id) == FVM_OK && id.end_frame == 7 && id.active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
