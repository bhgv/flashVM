/* Actual stream producer and two SWF event voices share exact mixed retry ownership. */
#define main mix_tutorial_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "../examples/multimedia/swf_stream_mix/main.c"

#undef main
#include "flashvm/swf_sound_mix_queue.h"
#include "flashvm/swf_stream_output.h"
#include <cstring>


struct Queue
{
    size_t count, calls, accepted;
    short saved[64];
    FVM_AudioQueueBackend *adapter;
    int fail, fail_clear, throwing, fail_observe;
};


/* Compare actual accepted samples later; a failed backend must accept no frames. */
static FVM_Result write_pcm(void *raw, const short *pcm, size_t frames)
{
    Queue *q = (Queue *)raw;
    ++q->calls;
    size_t n = 91;
    if (!(q->adapter->queued(q->adapter->userdata, &n) == FVM_INVALID_ARGUMENT && n == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (q->throwing)
    {
        throw 7;
    }
    if (q->fail)
    {
        return FVM_ERROR;
    }
    if (!(q->accepted + frames <= 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memcpy(q->saved + q->accepted * 2, pcm, frames * 2 * sizeof(short));
    q->accepted += frames;
    q->count += frames;
    return FVM_OK;
}


static FVM_Result queued(void *p, size_t *out)
{
    Queue *q = (Queue *)p;
    *out = q->count;
    return q->fail_observe ? FVM_ERROR : FVM_OK;
}


static FVM_Result pause_native(void *, int)
{
    return FVM_OK;
}


static FVM_Result clear_native(void *p)
{
    Queue *q = (Queue *)p;
    if (q->fail_clear)
    {
        return FVM_ERROR;
    }
    q->count = 0;
    return FVM_OK;
}


int main(int argc, char **argv)
{
    bool throwing = argc == 1;
    if (!(argc == 1 || (argc == 2 && !std::strcmp(argv[1], "--no-throw"))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config config;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundSnapshot *source = 0;
    if (!(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixerConfig geometry = {sizeof(geometry), 48000, 2, 2, FVM_SWF_SOUND_MIXER_CONVERT};
    FVM_SWFSoundMixer *mixer = 0;
    if (!(fvm_swf_sound_mixer_create_configured(&geometry, &mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < 2; ++i)
    {
        if (!(fvm_swf_sound_mixer_submit(mixer, c, i) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    FVM_AudioQueueBackend backend;
    std::memset(&backend, 0, sizeof(backend));
    Queue q;
    std::memset(&q, 0, sizeof(q));
    backend.struct_size = sizeof(backend);
    backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    backend.sample_rate = 48000;
    backend.channels = 2;
    backend.userdata = &q;
    backend.write = write_pcm;
    backend.queued = queued;
    backend.pause = pause_native;
    backend.clear = clear_native;
    FVM_AudioQueueBackend adapter = backend, saved = backend;
    FVM_SWFSoundMixQueue *mix = 0;
    if (!(fvm_swf_sound_mix_queue_create(mixer, &backend, 0, &mix, &adapter) == FVM_INVALID_ARGUMENT && !mix &&
          !std::memcmp(&adapter, &saved, sizeof(saved))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_create(mixer, &backend, 2, &mix, &adapter) == FVM_OK && !q.calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.adapter = &adapter;
    FVM_SWFStreamConverter converter;
    if (!(fvm_swf_stream_converter_init(&converter, source, 48000, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamOutput output;
    short retry[4];
    if (!(fvm_swf_stream_output_init(&output, &converter, &adapter, retry, 4, 2, 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t accepted = 91;
    q.fail = 1;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_ERROR && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_info(mix, &info) == FVM_OK && info.pending_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short changed[4];
    std::memcpy(changed, retry, sizeof(changed));
    ++changed[0];
    size_t calls = q.calls;
    if (!(adapter.write(adapter.userdata, changed, 2) == FVM_INVALID_ARGUMENT && q.calls == calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail_clear = 1;
    if (!(adapter.clear(adapter.userdata) == FVM_ERROR && fvm_swf_sound_mix_queue_info(mix, &info) == FVM_OK &&
          info.pending_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamConverter cursor = converter;
    if (throwing)
    {
        q.throwing = 1;
        if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_ERROR && !accepted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        q.throwing = 0;
    }
    if (!(!std::memcmp(&cursor, &converter, sizeof(cursor))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail = 0;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && accepted == 2 && q.saved[0] == 30000 &&
          q.saved[1] == 30000 && q.saved[2] == -25125))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail_observe = 1;
    size_t untouched = 91;
    if (!(adapter.queued(adapter.userdata, &untouched) == FVM_ERROR && untouched == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail_observe = 0;
    for (unsigned guard = 0; guard < 16; ++guard)
    {
        q.count = 0;
        FVM_Result result = fvm_swf_stream_output_pump(&output, &accepted);
        if (result == FVM_NOT_FOUND)
        {
            break;
        }
        if (!(result == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(q.accepted == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Stream producer is now quiescent; the host explicitly authorizes seven tail frames. */
    short zero[4] = {0, 0, 0, 0};
    while (q.accepted < 16)
    {
        size_t frames = q.accepted == 15 ? 1 : 2;
        if (!(adapter.write(adapter.userdata, zero, frames) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const short expected[] = {
        30000, -25125, 11875, -10250, 250, -1875, 10500, 4125, -30000, -30000, -30000, -30000, -30000, -30000, 0, 0};
    for (size_t i = 0; i < 16; ++i)
    {
        if (!(q.saved[i * 2] == expected[i] && q.saved[i * 2 + 1] == expected[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* A later rejection is cancelled only on acknowledged clear, without rewinding voices. */
    q.fail = 1;
    q.fail_clear = 0;
    if (!(adapter.write(adapter.userdata, zero, 2) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(adapter.clear(adapter.userdata) == FVM_OK && fvm_swf_sound_mix_queue_info(mix, &info) == FVM_OK &&
          !info.pending_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(mix);
    fvm_swf_sound_mixer_close(mixer);
    fvm_swf_stream_sound_snapshot_close(source);
    return 0;
}
