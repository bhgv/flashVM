/* Tracked callbacks reject reentry; backend/exception/budget failures preserve prepared ownership. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"


int main(int argc, char **argv)
{
    bool exceptions = argc == 1;
    if (!(argc == 1 || (argc == 2 && !std::strcmp(argv[1], "--no-throw"))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    IdentityFixture f;
    f.fifo.probe = f.clone(1);
    f.fifo.reenter = 1;
    FVM_SWFSoundVoice *voice = f.clone(0), *saved = voice;
    f.fifo.fail_observe = 1;
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_ERROR && voice == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoiceInfo progress;
    progress.struct_size = sizeof(progress);
    if (!(fvm_swf_sound_voice_info(voice, &progress) == FVM_OK && !progress.frame_in_play))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueIdentityInfo info;
    info.struct_size = sizeof(info);
    info.end_frame = 91;
    int yes = 91;
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_ERROR && info.end_frame == 91 &&
          fvm_swf_sound_mix_queue_sound_active(f.queue, 7, &yes) == FVM_ERROR && yes == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_observe = 0;
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
    if (exceptions)
    {
        f.fifo.throw_mode = 1;
        if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_ERROR && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.throw_mode = 2;
        info.end_frame = 91;
        if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_ERROR && info.end_frame == 91))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.throw_mode = 3;
        if (!(f.adapted.clear(f.adapted.userdata) == FVM_ERROR && active(f.mixer) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.throw_mode = 0;
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && info.end_frame == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(0);
    saved = voice;
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, 0) == FVM_OUT_OF_MEMORY && voice == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_voice_close(voice);
    f.fifo.fail = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK && n == 2 && f.fifo.saved[0] == 100 &&
          f.fifo.saved[1] == 101))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f.adapted.clear(f.adapted.userdata) == FVM_OK && !active(f.mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && !info.active && !info.end_frame &&
          !info.played_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_info(f.fifo.probe, &progress) == FVM_OK && !progress.frame_in_play))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
