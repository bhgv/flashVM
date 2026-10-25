/* Neither producer can steal the other producer's already rendered retry. */
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
    FVM_SWFSoundMixQueuePending p;
    p.struct_size = sizeof(p);
    if (!(fvm_swf_sound_mix_queue_pending(queue, &p) == FVM_OK && !p.frames &&
          p.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_NONE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short zeros[2] = {0, 0};
    q.fail = 1;
    if (!(adapted.write(adapted.userdata, zeros, 2) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pending(queue, &p) == FVM_OK && p.frames == 2 &&
          p.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t accepted = 91, calls = q.calls;
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_INVALID_ARGUMENT && !accepted &&
          q.calls == calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail = 0;
    if (!(adapted.write(adapted.userdata, zeros, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Samples samples;
    samples.push_back(7);
    samples.push_back(8);
    Bytes swf = movie(samples, 3, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK && fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    q.count = 0;
    q.fail = 1;
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_ERROR && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pending(queue, &p) == FVM_OK && p.frames == 2 &&
          p.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    calls = q.calls;
    if (!(adapted.write(adapted.userdata, zeros, 2) == FVM_INVALID_ARGUMENT && q.calls == calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 1, 4, &accepted) == FVM_INVALID_ARGUMENT && !accepted &&
          q.calls == calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_OK && accepted == 2 && q.saved[2] == 7 &&
          q.saved[3] == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    p.struct_size = 0;
    p.frames = 91;
    p.origin = 91;
    if (!(fvm_swf_sound_mix_queue_pending(queue, &p) == FVM_INVALID_ARGUMENT && p.frames == 91 && p.origin == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(queue);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
