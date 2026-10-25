/* Exact-fit creation succeeds; insufficient budgets and invalid descriptors publish no ownership. */
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
    FVM_AudioQueueBackend adapted = b;
    FVM_SWFSoundMixQueue *queue = (FVM_SWFSoundMixQueue *)1;
    size_t required = 0;
    if (!(fvm_swf_sound_mix_queue_storage_required(1, 16, &required) == FVM_OK && required))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_create_bounded(mixer, &b, 16, 0, &queue, &adapted) == FVM_OUT_OF_MEMORY && !queue &&
          !std::memcmp(&b, &adapted, sizeof(b)) && !q.calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_create_bounded(mixer, &b, 16, required - 1, &queue, &adapted) == FVM_OUT_OF_MEMORY &&
          !queue && !std::memcmp(&b, &adapted, sizeof(b)) && !q.calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_create_bounded(mixer, &b, 0, 0, &queue, &adapted) == FVM_INVALID_ARGUMENT && !queue))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    b.channels = 2;
    if (!(fvm_swf_sound_mix_queue_create_bounded(mixer, &b, 16, 0, &queue, &adapted) == FVM_INVALID_ARGUMENT && !queue))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    b.channels = 1;
    if (!(fvm_swf_sound_mix_queue_create_bounded(mixer, &b, 16, required, &queue, &adapted) == FVM_OK && queue &&
          !q.calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueStorageInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_storage_info(queue, &info) == FVM_OK && info.retained_bytes == required))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(queue);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
