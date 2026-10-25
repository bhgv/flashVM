/* Tail pumping retains one render across rejection and stops producing after voice exhaustion. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_conversion_fixture.hpp"
#include "flashvm/swf_sound_mix_queue.h"

using namespace conversion_fixture;


struct TailQueue
{
    size_t count, calls, total;
    int fail;
    short saved[32];
};


static FVM_Result tail_write(void *p, const short *pcm, size_t frames)
{
    TailQueue *q = (TailQueue *)p;
    ++q->calls;
    if (q->fail)
    {
        return FVM_ERROR;
    }
    if (!(q->total + frames <= 32))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memcpy(q->saved + q->total, pcm, frames * sizeof(short));
    q->total += frames;
    q->count += frames;
    return FVM_OK;
}


static FVM_Result tail_count(void *p, size_t *out)
{
    *out = ((TailQueue *)p)->count;
    return FVM_OK;
}


static FVM_Result tail_pause(void *, int)
{
    return FVM_OK;
}


static FVM_Result tail_clear(void *p)
{
    ((TailQueue *)p)->count = 0;
    return FVM_OK;
}


int main()
{
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
    for (unsigned i = 0; i < 5; ++i)
    {
        samples.push_back((short)(100 + i));
    }
    Bytes swf = movie(samples, 3, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *mixer = create(44100, 1);
    if (!(fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
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
    size_t accepted = 91;
    q.fail = 1;
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_ERROR && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_OK && accepted == 2 && q.saved[0] == 100 &&
          q.saved[1] == 101))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_OK && accepted == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_NOT_FOUND && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_OK && q.saved[4] == 104 && q.saved[5] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_NOT_FOUND && !accepted && q.count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(queue);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
