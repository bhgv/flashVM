/* Native movie-frame authorization holds future blocks and exact pending retries without decode. */
#define main stream_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_sound.cpp"

#undef main
#include "flashvm/swf_stream_output.h"


struct GateQueue
{
    size_t frames, writes;
    int fail;
};


/* The test backend atomically accepts complete native frames; consumption is controlled separately. */
static FVM_Result gate_write(void *raw, const short *, size_t n)
{
    GateQueue *q = (GateQueue *)raw;
    ++q->writes;
    if (q->fail)
    {
        return FVM_ERROR;
    }
    q->frames += n;
    return FVM_OK;
}


static FVM_Result gate_count(void *raw, size_t *n)
{
    *n = ((GateQueue *)raw)->frames;
    return FVM_OK;
}


static FVM_Result gate_pause(void *, int)
{
    return FVM_OK;
}


static FVM_Result gate_clear(void *raw)
{
    ((GateQueue *)raw)->frames = 0;
    return FVM_OK;
}


/* Check native source identity, pending precedence and EOF separately from a nonempty FIFO. */
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
    Bytes tags, data, empty;
    data.push_back(0);
    data.push_back(255);
    tag(tags, 45, header(12));
    tag(tags, 19, data);
    tag(tags, 1, empty);
    tag(tags, 1, empty);
    data.clear();
    data.push_back(128);
    tag(tags, 19, data);
    tag(tags, 1, empty);
    tag(tags, 19, empty);
    load(c, stream_movie(tags, 8, 4));
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &snapshot) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    FVM_SWFStreamConverter converter;
    if (!(fvm_swf_stream_converter_init(&converter, snapshot, 44100, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    GateQueue queue = {0, 0, 0};
    FVM_AudioQueueBackend backend;
    std::memset(&backend, 0, sizeof(backend));
    backend.struct_size = sizeof(backend);
    backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    backend.sample_rate = 44100;
    backend.channels = 1;
    backend.userdata = &queue;
    backend.write = gate_write;
    backend.queued = gate_count;
    backend.pause = gate_pause;
    backend.clear = gate_clear;
    short pcm = 42;
    FVM_SWFStreamOutput output;
    if (!(fvm_swf_stream_output_init(&output, &converter, &backend, &pcm, 1, 1, 8) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned frame = 77;
    size_t accepted = 999;
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_OK && frame == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamConverter before = converter;
    if (!(fvm_swf_stream_output_pump_until(&output, 0, &accepted) == FVM_NOT_FOUND && !accepted && pcm == 42 &&
          !queue.writes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(&before, &converter, sizeof(before))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_until(&output, 1, &accepted) == FVM_OK && accepted == 1 && pcm == -32768))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_until(&output, 1, &accepted) == FVM_OK && accepted == 1 && pcm == 32512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_OK && frame == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = converter;
    size_t writes = queue.writes;
    if (!(fvm_swf_stream_output_pump_until(&output, 2, &accepted) == FVM_NOT_FOUND && !accepted &&
          queue.writes == writes && !std::memcmp(&before, &converter, sizeof(before))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.fail = 1;
    if (!(fvm_swf_stream_output_pump_until(&output, 3, &accepted) == FVM_ERROR && !accepted &&
          output.info.pending_frames == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = converter;
    short pending = pcm;
    writes = queue.writes;
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_OK && frame == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_until(&output, 2, &accepted) == FVM_NOT_FOUND && pcm == pending &&
          queue.writes == writes && !std::memcmp(&before, &converter, sizeof(before))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    queue.fail = 0;
    if (!(fvm_swf_stream_output_pump_until(&output, 3, &accepted) == FVM_OK && accepted == 1 &&
          !std::memcmp(&before, &converter, sizeof(before))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_OK && frame == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_until(&output, 3, &accepted) == FVM_NOT_FOUND && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_until(&output, 4, &accepted) == FVM_OK && !accepted &&
          (output.info.chunk.flags & FVM_STREAM_CONVERT_EMPTY)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    frame = 77;
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_NOT_FOUND && frame == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_until(&output, 4, &accepted) == FVM_NOT_FOUND && output.info.eof &&
          output.info.queued_frames == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek(&output, 0, 0) == FVM_OK && !queue.frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_OK && frame == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump(&output, &accepted) == FVM_OK && accepted == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = converter;
    writes = queue.writes;
    if (!(fvm_swf_stream_output_seek_frame(&output, 0) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&before, &converter, sizeof(before)) && queue.writes == writes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek_frame(&output, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_OK && frame == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_until(&output, 2, &accepted) == FVM_NOT_FOUND && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek_frame(&output, 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_OK && frame == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek_frame(&output, 5) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    frame = 77;
    if (!(fvm_swf_stream_output_next_frame(&output, &frame) == FVM_NOT_FOUND && frame == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(snapshot);
    return 0;
}
