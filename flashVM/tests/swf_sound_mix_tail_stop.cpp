/* Stop controls affect future composition; pending output survives, and DECLICK finishes exactly. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_conversion_fixture.hpp"
#include "flashvm/swf_sound_mix_queue.h"

using namespace conversion_fixture;


struct StopQueue
{
    short pcm[512];
    size_t count, total, calls;
    int fail;
};


static FVM_Result stop_write(void *raw, const short *pcm, size_t frames)
{
    StopQueue *q = (StopQueue *)raw;
    ++q->calls;
    if (q->fail)
    {
        return FVM_ERROR;
    }
    if (!(q->total + frames <= 512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memcpy(q->pcm + q->total, pcm, frames * sizeof(short));
    q->total += frames;
    q->count += frames;
    return FVM_OK;
}


static FVM_Result stop_count(void *raw, size_t *out)
{
    *out = ((StopQueue *)raw)->count;
    return FVM_OK;
}


static FVM_Result stop_pause(void *, int)
{
    return FVM_OK;
}


static FVM_Result stop_clear(void *raw)
{
    ((StopQueue *)raw)->count = 0;
    return FVM_OK;
}


int main()
{
    for (unsigned mode = 0; mode < 3; ++mode)
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
        for (unsigned i = 0; i < 512; ++i)
        {
            samples.push_back(1000);
        }
        Bytes swf = movie(samples, 3, 1);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixerConfig config = {sizeof(config), 44100, 1, 2, mode == 1 ? FVM_SWF_SOUND_MIXER_DECLICK : 0};
        FVM_SWFSoundMixer *mixer = 0;
        if (!(fvm_swf_sound_mixer_create_configured(&config, &mixer) == FVM_OK &&
              fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
        StopQueue q;
        std::memset(&q, 0, sizeof(q));
        FVM_AudioQueueBackend b;
        std::memset(&b, 0, sizeof(b));
        b.struct_size = sizeof(b);
        b.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
        b.sample_rate = 44100;
        b.channels = 1;
        b.userdata = &q;
        b.write = stop_write;
        b.queued = stop_count;
        b.pause = stop_pause;
        b.clear = stop_clear;
        FVM_AudioQueueBackend adapted;
        FVM_SWFSoundMixQueue *queue = 0;
        if (!(fvm_swf_sound_mix_queue_create(mixer, &b, 16, &queue, &adapted) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t accepted = 91;
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 0, 16, &accepted) == FVM_INVALID_ARGUMENT && !accepted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 17, 32, &accepted) == FVM_INVALID_ARGUMENT && !accepted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 16, 15, &accepted) == FVM_INVALID_ARGUMENT && !accepted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 16, FVM_SWF_SOUND_OUTPUT_MAX_QUEUE + 1, &accepted) ==
                  FVM_INVALID_ARGUMENT &&
              !accepted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        q.count = (size_t)-1;
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 16, 32, &accepted) == FVM_INVALID_ARGUMENT && !accepted &&
              !q.calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        q.count = 0;
        q.fail = 1;
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 16, 32, &accepted) == FVM_ERROR && !accepted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!((mode == 2 ? fvm_swf_sound_mixer_stop_all(mixer) : fvm_swf_sound_mixer_stop_id(mixer, 7)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        q.fail = 0;
        if (mode == 2)
        {
            if (!(adapted.clear(adapted.userdata) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 16, 32, &accepted) == FVM_OK && accepted == 16))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        for (unsigned guard = 0; guard < 32; ++guard)
        {
            q.count = 0;
            FVM_Result result = fvm_swf_sound_mix_queue_pump_tail(queue, 16, 32, &accepted);
            if (result == FVM_NOT_FOUND)
            {
                break;
            }
            if (!(result == FVM_OK && accepted == 16))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(q.total == (mode == 1 ? 240 : mode == 0 ? 16 : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < q.total; ++i)
        {
            long expected = i < 16 ? 1000 : mode == 1 && i < 236 ? 1000L * (235 - (long)i) / 219 : 0;
            if (!(q.pcm[i] == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 16, 32, &accepted) == FVM_NOT_FOUND && !accepted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_sound_mix_queue_close(queue);
        fvm_swf_sound_mixer_close(mixer);
    }
    return 0;
}
