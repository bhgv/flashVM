/* A cancelled ID lease permits a new NoMultiple voice while the old DECLICK tail releases. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"


int main()
{


    IdentityFixture f(true, 100, FVM_SWF_SOUND_MIXER_CONVERT | FVM_SWF_SOUND_MIXER_DECLICK);
    FVM_SWFSoundVoice *voice = f.clone(0);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t n = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &n) == FVM_OK && f.fifo.saved[0] == 100))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_stop_id(f.queue, 7) == FVM_OK && active(f.mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(1);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK && !voice &&
          active(f.mixer) == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueIdentityInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && info.end_frame == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &n) == FVM_OK && f.fifo.saved[1] == 201))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
