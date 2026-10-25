/* ID stop preserves retained PCM; acknowledged tracked clear cancels voices, deadlines and epoch. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"


int main()
{
    IdentityFixture f;
    FVM_SWFSoundVoice *voice = f.clone(0);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short zero[2] = {0, 0};
    f.fifo.fail = 1;
    if (!(f.adapted.write(f.adapted.userdata, zero, 2) == FVM_ERROR &&
          fvm_swf_sound_mix_queue_stop_id(f.queue, 7) == FVM_OK && !active(f.mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueIdentityInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && !info.active && !info.end_frame))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(1);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK && !voice))
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
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &accepted) == FVM_OK && f.fifo.saved[2] == 100 &&
          f.fifo.saved[3] == 101))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && info.end_frame == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    IdentityFixture g;
    voice = g.clone(0);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(g.queue, &voice, (size_t)-1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g.fifo.fail = 1;
    if (!(g.adapted.write(g.adapted.userdata, zero, 2) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g.fifo.fail_clear = 1;
    if (!(g.adapted.clear(g.adapted.userdata) == FVM_ERROR && active(g.mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(g.queue, 7, &info) == FVM_OK && info.end_frame == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueuePending pending;
    pending.struct_size = sizeof(pending);
    if (!(fvm_swf_sound_mix_queue_pending(g.queue, &pending) == FVM_OK && pending.frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g.fifo.fail_clear = 0;
    if (!(g.adapted.clear(g.adapted.userdata) == FVM_OK && !active(g.mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(g.queue, 7, &info) == FVM_OK && !info.end_frame && !info.played_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pending(g.queue, &pending) == FVM_OK && !pending.frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_set_identity_tracking(g.queue, 0, 0) == FVM_OK &&
          fvm_swf_sound_mix_queue_clock_end(g.queue) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
