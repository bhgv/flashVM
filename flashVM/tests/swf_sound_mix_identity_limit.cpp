/* White-box exact counter boundary; rejected admission never consumes an incoming voice. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/swf/sound_mix_queue.cpp"
#include "swf_sound_mix_identity_fixture.hpp"


int main()
{
    IdentityFixture f;
    f.queue->clock_submitted = f.queue->clock_played = FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - 4;
    FVM_SWFSoundVoice *voice = f.clone(0), *saved = voice;
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_UNSUPPORTED &&
          voice == saved))
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
    f.queue->clock_submitted = f.queue->clock_played = FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - 5;
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
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, 0) == FVM_OK && !voice))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(0);
    saved = voice;
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_UNSUPPORTED &&
          voice == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_voice_close(voice);
    f.fifo.fail = 0;
    if (!(f.adapted.write(f.adapted.userdata, zero, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t n = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 2, 8, &n) == FVM_OK &&
          fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &n) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueIdentityInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK &&
          info.end_frame == FVM_SWF_SOUND_OUTPUT_MAX_CLOCK && info.until_end_frames == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 0;
    if (!(fvm_swf_sound_mix_queue_identity_info(f.queue, 7, &info) == FVM_OK && !info.active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = f.clone(1);
    saved = voice;
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_UNSUPPORTED &&
          voice == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f.adapted.clear(f.adapted.userdata) == FVM_OK &&
          fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue, &voice, (size_t)-1) == FVM_OK && !voice))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(f.queue, 1, 8, &n) == FVM_OK && f.fifo.saved[5] == 100))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    IdentityFixture g;
    voice = g.clone(0);
    short pcm[5];
    size_t written = 0;
    if (!(fvm_swf_sound_voice_read(voice, 5, pcm, 5, &written) == FVM_OK && written == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(g.queue, &voice, 0) == FVM_OK && !voice &&
          fvm_swf_sound_mix_queue_identity_info(g.queue, 7, &info) == FVM_OK && !info.end_frame))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice = g.clone(0);
    if (!(fvm_swf_sound_voice_read(voice, 2, pcm, 5, &written) == FVM_OK && written == 2 &&
          fvm_swf_sound_mix_queue_submit_prepared_bounded(g.queue, &voice, (size_t)-1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(g.queue, 7, &info) == FVM_OK && info.end_frame == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(g.queue, 2, 8, &n) == FVM_OK &&
          fvm_swf_sound_mix_queue_pump_tail(g.queue, 1, 8, &n) == FVM_OK && g.fifo.saved[0] == 102 &&
          g.fifo.saved[1] == 103 && g.fifo.saved[2] == 104))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
