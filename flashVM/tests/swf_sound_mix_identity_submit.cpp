/* NoMultiple covers pending and queued PCM after mixer retirement, then permits a drained restart. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"


int main()
{
    IdentityFixture f;
    FVM_SWFSoundVoice *voice = f.clone(0);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK && !voice))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short zero[2] = {0, 0};
    f.fifo.fail = 1;
    if (!(f.adapted.write(f.adapted.userdata, zero, 2) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(1);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, 0) == FVM_OK && !voice &&
          active(f.mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail = 0;
    if (!(f.adapted.write(f.adapted.userdata, zero, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t accepted = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &accepted) == FVM_OK && accepted == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &accepted) == FVM_OK && accepted == 1 &&
          active(f.mixer) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(1);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, 0) == FVM_OK && !voice &&
          active(f.mixer) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 5; ++i)
    {
        if (!(f.fifo.saved[i] == 100 + i))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    f.fifo.count = 0;
    voice = f.clone(1);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK && !voice &&
          active(f.mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &accepted) == FVM_OK && f.fifo.saved[5] == 100))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
