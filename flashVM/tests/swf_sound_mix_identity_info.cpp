/* Deadline extension starts after pending PCM; failed adoption and bad queries preserve state. */
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
    if (!(f.adapted.write(f.adapted.userdata, zero, 2) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(0);
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK && !voice))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueIdentityInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && info.end_frame == 7 &&
          info.until_end_frames == 7 && info.active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(0);
    FVM_SWFSoundVoice *saved = voice;
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, 0) == FVM_OUT_OF_MEMORY && voice == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoiceInfo state;
    state.struct_size = sizeof(state);
    if (!(fvm_swf_sound_voice_info(voice, &state) == FVM_OK && !state.frame_in_play))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_voice_close(voice);
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && info.end_frame == 7))
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
    size_t n = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK &&
          fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK &&
          fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &n) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const short expected[] = {100, 101, 202, 204, 206, 103, 104};
    if (!(f.fifo.total == 7 && !std::memcmp(f.fifo.saved, expected, sizeof(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && info.active && !info.played_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 4;
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && info.played_frames == 3 &&
          info.until_end_frames == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 0;
    int yes = 91;
    if (!(fvm_swf_sound_mix_queue_sound_active(f.queue, 7, &yes) == FVM_OK && !yes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 65535, &info) == FVM_OK && !info.active && !info.end_frame))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.end_frame = 91;
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 0, &info) == FVM_INVALID_ARGUMENT && info.end_frame == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 65536, &info) == FVM_INVALID_ARGUMENT && info.end_frame == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    IdentityFixture disabled(false);
    yes = 91;
    if (!(fvm_swf_sound_mix_queue_identity_info(disabled.queue, 7, &info) == FVM_UNSUPPORTED && info.end_frame == 91 &&
          fvm_swf_sound_mix_queue_sound_active(disabled.queue, 7, &yes) == FVM_UNSUPPORTED && yes == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
