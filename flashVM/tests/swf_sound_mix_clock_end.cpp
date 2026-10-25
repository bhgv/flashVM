/* Drained termination restores the legacy backend contract; pending/unplayed data cannot end it. */
#define main tail_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_tail.cpp"

#undef main


int main()
{
    FVM_SWFSoundMixer *mixer = create(44100, 1);
    TailQueue q;
    std::memset(&q, 0, sizeof(q));
    FVM_AudioQueueBackend b;
    std::memset(&b, 0, sizeof(b));
    b.struct_size = sizeof(b);
    b.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    b.sample_rate = 44100;
    b.channels = 1;
    b.userdata = &q;
    b.write = tail_write;
    b.queued = tail_count;
    b.pause = tail_pause;
    b.clear = tail_clear;
    FVM_AudioQueueBackend adapted;
    FVM_SWFSoundMixQueue *queue = 0;
    if (!(fvm_swf_sound_mix_queue_create(mixer, &b, 2, &queue, &adapted) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_clock_end(queue) == FVM_NOT_FOUND &&
          fvm_swf_sound_mix_queue_clock_begin(queue) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short pcm[] = {1, 2};
    q.fail = 1;
    if (!(adapted.write(adapted.userdata, pcm, 2) == FVM_ERROR &&
          fvm_swf_sound_mix_queue_clock_end(queue) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail = 0;
    if (!(adapted.write(adapted.userdata, pcm, 2) == FVM_OK &&
          fvm_swf_sound_mix_queue_clock_end(queue) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 0;
    if (!(fvm_swf_sound_mix_queue_clock_end(queue) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueClock clock;
    clock.struct_size = sizeof(clock);
    clock.submitted_frames = 91;
    if (!(fvm_swf_sound_mix_queue_clock_info(queue, &clock) == FVM_NOT_FOUND && clock.submitted_frames == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 123;
    size_t count = 0;
    if (!(adapted.queued(adapted.userdata, &count) == FVM_OK && count == 123))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_clock_begin(queue) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 0;
    if (!(fvm_swf_sound_mix_queue_clock_begin(queue) == FVM_OK &&
          fvm_swf_sound_mix_queue_clock_info(queue, &clock) == FVM_OK && !clock.submitted_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(queue);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
