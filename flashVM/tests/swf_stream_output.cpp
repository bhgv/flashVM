/* Bounded native FIFO producer: exact retries, queue clocks, callback failures/reentry and seek. */
#define main stream_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_sound.cpp"

#undef main
#include "flashvm/swf_stream_output.h"


struct Queue
{
    size_t count, writes, pauses, clears;
    int paused, fail, fail_clear, throw_write;
    short saved[8];
    size_t size;
    FVM_SWFStreamOutput *reenter;
};


/* All-or-none backend stores an exact witness even for deliberately rejected writes. */
static FVM_Result write(void *raw, const short *pcm, size_t frames)
{
    Queue *q = (Queue *)raw;
    ++q->writes;
    if (q->reenter)
    {
        size_t n = 999;
        if (!(fvm_swf_stream_output_pump(q->reenter, &n) == FVM_INVALID_ARGUMENT && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (q->size)
    {
        if (!(q->size == frames && !std::memcmp(q->saved, pcm, frames * 2 * sizeof(short))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        q->size = frames;
        std::memcpy(q->saved, pcm, frames * 2 * sizeof(short));
    }
    if (q->throw_write)
    {
        throw 1;
    }
    if (q->fail)
    {
        return FVM_ERROR;
    }
    q->size = 0;
    q->count += frames;
    return FVM_OK;
}


/* Backend owns FIFO observations; tests explicitly simulate consumption and violations. */
static FVM_Result queued(void *raw, size_t *n)
{
    *n = ((Queue *)raw)->count;
    return FVM_OK;
}


static FVM_Result pause(void *raw, int value)
{
    Queue *q = (Queue *)raw;
    ++q->pauses;
    q->paused = value;
    return FVM_OK;
}


static FVM_Result clear(void *raw)
{
    Queue *q = (Queue *)raw;
    ++q->clears;
    if (q->fail_clear)
    {
        return FVM_ERROR;
    }
    q->count = 0;
    q->size = 0;
    return FVM_OK;
}


/* Source bytes and snapshot survive original VM destruction; producer never reaches a context. */
int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes tags, data, empty;
    tag(tags, 45, header(0x3d));
    data.push_back(0);
    data.push_back(255);
    data.push_back(128);
    data.push_back(128);
    tag(tags, 19, data);
    tag(tags, 1, empty);
    tag(tags, 19, empty);
    load(context, stream_movie(tags, 8, 2));
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    if (!(fvm_swf_stream_sound_snapshot_open(context, 0, &snapshot) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    FVM_SWFStreamConverter converter;
    if (!(fvm_swf_stream_converter_init(&converter, snapshot, 48000, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_AudioQueueBackend backend;
    std::memset(&backend, 0, sizeof(backend));
    Queue queue;
    std::memset(&queue, 0, sizeof(queue));
    backend.struct_size = sizeof(backend);
    backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    backend.sample_rate = 48000;
    backend.channels = 2;
    backend.userdata = &queue;
    backend.write = write;
    backend.queued = queued;
    backend.pause = pause;
    backend.clear = clear;
    short pcm[4] = {42, 42, 42, 42};
    FVM_SWFStreamOutput output, untouched;
    std::memset(&output, 0xa5, sizeof(output));
    untouched = output;
    if (!(fvm_swf_stream_output_init(&output, &converter, &backend, pcm, 3, 2, 2) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&output, &untouched, sizeof(output))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.count = 1;
    if (!(fvm_swf_stream_output_init(&output, &converter, &backend, pcm, 4, 2, 2) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&output, &untouched, sizeof(output))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.count = 0;
    if (!(fvm_swf_stream_output_init(&output, &converter, &backend, pcm, 4, 2, 2) == FVM_OK && queue.paused))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.reenter = &output;
    queue.fail = 1;
    size_t accepted = 999;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_ERROR && !accepted && output.info.pending_frames == 2 &&
          converter.first == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamConverter position = converter;
    short retry[4];
    std::memcpy(retry, pcm, sizeof(retry));
    queue.fail = 0;
    queue.throw_write = 1;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_ERROR && !accepted && !output.busy))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(&position, &converter, sizeof(position)) && !std::memcmp(retry, pcm, sizeof(retry))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.throw_write = 0;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && accepted == 2 && queue.count == 2 &&
          !output.info.pending_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position = converter;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_NOT_FOUND && !accepted &&
          !std::memcmp(&position, &converter, sizeof(position))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pause(&output, 0) == FVM_OK && !queue.paused))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.count = 1;
    FVM_SWFStreamOutputInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_stream_output_info(&output, &info) == FVM_OK && info.submitted_frames == 2 &&
          info.played_frames == 1 && info.queued_frames == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamOutputInfo saved = info;
    queue.count = 2;
    if (!(fvm_swf_stream_output_info(&output, &info) == FVM_ERROR && !std::memcmp(&info, &saved, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.count = 1;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && accepted == 1 && queue.count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.count = 0;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && !accepted &&
          (output.info.chunk.flags & FVM_STREAM_CONVERT_EMPTY)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_NOT_FOUND && output.info.eof))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_info(&output, &info) == FVM_OK && info.submitted_frames == 3 &&
          info.played_frames == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.fail_clear = 1;
    position = converter;
    if (!(fvm_swf_stream_output_seek(&output, 0, 0) == FVM_ERROR && output.info.paused && output.info.eof &&
          !std::memcmp(&position, &converter, sizeof(position))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.fail_clear = 0;
    if (!(fvm_swf_stream_output_seek(&output, 0, 0) == FVM_OK && output.info.submitted_frames == 0 &&
          !output.info.eof && converter.first == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t clears = queue.clears;
    if (!(fvm_swf_stream_output_seek(&output, 99, 0) == FVM_INVALID_ARGUMENT && queue.clears == clears))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.fail = 1;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_ERROR && output.info.pending_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.fail_clear = 1;
    position = converter;
    std::memcpy(retry, pcm, sizeof(retry));
    if (!(fvm_swf_stream_output_seek(&output, 0, 0) == FVM_ERROR && output.info.pending_frames == 2 &&
          output.info.paused))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(&position, &converter, sizeof(position)) && !std::memcmp(retry, pcm, sizeof(retry))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.fail_clear = 0;
    if (!(fvm_swf_stream_output_seek(&output, 0, 0) == FVM_OK && !output.info.pending_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.fail = 0;
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && accepted == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek(&output, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_init(&output, &converter, &backend, pcm, 4, 2, 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && accepted == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && accepted == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_NOT_FOUND && output.info.eof))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_info(&output, &info) == FVM_OK && info.eof && info.queued_frames == 3 &&
          info.played_frames == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pause(&output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.count = 0;
    if (!(fvm_swf_stream_output_info(&output, &info) == FVM_OK && info.played_frames == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(snapshot);
    return 0;
}
