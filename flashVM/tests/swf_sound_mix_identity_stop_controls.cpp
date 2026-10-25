/* Stop-all has no backend dependency, rejects recursion and preserves disabled owners. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"


int main()
{
    if (!(fvm_swf_sound_mix_queue_stop_all(0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    IdentityFixture plain(false);
    FVM_SWFSoundVoice *v = plain.clone(0);
    if (!(fvm_swf_sound_mixer_submit_prepared_bounded(plain.mixer, &v, (size_t)-1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_stop_all(plain.queue) == FVM_UNSUPPORTED && active(plain.mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    IdentityFixture f;
    v = f.clone(0);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &v, (size_t)-1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.reenter = 1;
    f.fifo.probe = f.clone(1);
    size_t n = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &n) == FVM_OK && active(f.mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.throw_mode = 2;
    f.fifo.fail_observe = 1;
    f.fifo.fail_clear = 1;
    size_t calls = f.fifo.calls;
    if (!(fvm_swf_sound_mix_queue_stop_all(f.queue) == FVM_OK && !active(f.mixer) && f.fifo.calls == calls &&
          f.fifo.count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.throw_mode = 0;
    f.fifo.fail_observe = 0;
    FVM_SWFSoundMixQueueIdentityInfo id;
    id.struct_size = sizeof(id);
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &id) == FVM_OK && !id.end_frame))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
