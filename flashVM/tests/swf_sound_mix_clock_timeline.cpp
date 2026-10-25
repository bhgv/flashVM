/* Actual MovieClip interpreter/JIT authorization, stopped ticks, GC and seek reset the shared FIFO epoch. */
#define main stream_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_sound.cpp"

#undef main
#include "flashvm/swf_stream_output.h"
#include "flashvm/swf_sound_mix_queue.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/execution_mode.h"
#include <cstdio>


struct TimelineQueue
{
    size_t queued, total;
    short samples[64];
    int paused;
};


/* All-or-none native capture records only authorized PCM, independently of VM timeline state. */
static FVM_Result timeline_write(void *raw, const short *pcm, size_t frames)
{
    TimelineQueue *q = (TimelineQueue *)raw;
    if (!((q->total + frames) * 2 <= 64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memcpy(q->samples + q->total * 2, pcm, frames * 2 * sizeof(short));
    q->total += frames;
    q->queued += frames;
    return FVM_OK;
}


static FVM_Result timeline_count(void *raw, size_t *n)
{
    *n = ((TimelineQueue *)raw)->queued;
    return FVM_OK;
}


static FVM_Result timeline_pause(void *raw, int value)
{
    ((TimelineQueue *)raw)->paused = value;
    return FVM_OK;
}


static FVM_Result timeline_clear(void *raw)
{
    ((TimelineQueue *)raw)->queued = 0;
    return FVM_OK;
}


/* Raw AVM1 assigns a distinct visit number, ensuring actual interpreter/JIT frame actions run. */
static void visit(Bytes &tags, unsigned frame)
{
    Bytes action;
    action.push_back(0x96);
    word(action, 14);
    action.push_back(0);
    const char *name = "visited";
    for (unsigned i = 0; i < 8; ++i)
    {
        action.push_back((unsigned char)name[i]);
    }
    action.push_back(7);
    integer(action, frame);
    action.push_back(0x1d);
    action.push_back(0);
    tag(tags, 12, action);
}


/* Read the actual playhead, not elapsed host tick or presentation counters. */
static unsigned frame(FVM_Context *c, FVM_Object *clip)
{
    FVM_Value v;
    if (!(fvm_object_get(c, clip, "_currentframe", &v) == FVM_OK && v.type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return (unsigned)v.number;
}


static void method(FVM_Context *c, FVM_Object *clip, const char *name, unsigned target = 0)
{
    FVM_Value arg, result;
    fvm_value_number(&arg, target);
    if (!(fvm_object_call(c, clip, name, target ? 1 : 0, target ? &arg : 0, &result) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static size_t pump_frame(FVM_SWFStreamOutput *output, unsigned authorized)
{
    size_t total = 0, n;
    for (unsigned i = 0; i < 16; ++i)
    {
        FVM_Result r = fvm_swf_stream_output_pump_until(output, authorized, &n);
        if (r == FVM_NOT_FOUND)
        {
            return total;
        }
        if (!(r == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        total += n;
    }
    if (!(false))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}


/* Retained native source and pinned root are coordinated only at explicit owner safe points. */
int main()
{
    unsigned executed = 0;
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        if (mode && !fvm_jit_available())
        {
            continue;
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
        Bytes tags, data, empty;
        tag(tags, 45, header(0x3d));
        data.push_back(0);
        data.push_back(255);
        data.push_back(128);
        data.push_back(128);
        tag(tags, 19, data);
        visit(tags, 1);
        tag(tags, 1, empty);
        visit(tags, 2);
        tag(tags, 1, empty);
        data.clear();
        data.push_back(255);
        data.push_back(0);
        tag(tags, 19, data);
        visit(tags, 3);
        tag(tags, 1, empty);
        tag(tags, 19, empty);
        visit(tags, 4);
        Bytes movie = stream_movie(tags, 8, 4);
        load(c, movie);
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamSoundSnapshot *snapshot = 0;
        if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &snapshot) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *clip = 0;
        if (!(fvm_movieclip_load_swf_ex(
                  c, movie.begin(), movie.size(), "_root", FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) == FVM_OK &&
              fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamConverter converter;
        if (!(fvm_swf_stream_converter_init(&converter, snapshot, 48000, 2) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        TimelineQueue queue;
        std::memset(&queue, 0, sizeof(queue));
        FVM_AudioQueueBackend backend;
        std::memset(&backend, 0, sizeof(backend));
        backend.struct_size = sizeof(backend);
        backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
        backend.sample_rate = 48000;
        backend.channels = 2;
        backend.userdata = &queue;
        backend.write = timeline_write;
        backend.queued = timeline_count;
        backend.pause = timeline_pause;
        backend.clear = timeline_clear;
        FVM_SWFSoundMixerConfig geometry = {sizeof(geometry), 48000, 2, 1, FVM_SWF_SOUND_MIXER_CONVERT};
        FVM_SWFSoundMixer *mixer = 0;
        if (!(fvm_swf_sound_mixer_create_configured(&geometry, &mixer) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueue *composition = 0;
        FVM_AudioQueueBackend adapted;
        if (!(fvm_swf_sound_mix_queue_create(mixer, &backend, 1, &composition, &adapted) == FVM_OK &&
              fvm_swf_sound_mix_queue_clock_begin(composition) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueueClock shared;
        shared.struct_size = sizeof(shared);
        FVM_SWFStreamOutput output;
        short retry[2];
        if (!(fvm_swf_stream_output_init(&output, &converter, &adapted, retry, 2, 1, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pump_frame(&output, 0) == 0 && queue.total == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && frame(c, clip) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pump_frame(&output, frame(c, clip)) == 3 && queue.total == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_clock_info(composition, &shared) == FVM_OK && shared.submitted_frames == 3 &&
              shared.played_frames == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        method(c, clip, "stop");
        if (!(fvm_swf_stream_output_pause(&output, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 2) == FVM_OK && frame(c, clip) == 1 &&
              pump_frame(&output, frame(c, clip)) == 0 && queue.total == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_step(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        method(c, clip, "play");
        if (!(fvm_swf_stream_output_pause(&output, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        queue.queued = 0;
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && frame(c, clip) == 2 &&
              pump_frame(&output, frame(c, clip)) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_clock_info(composition, &shared) == FVM_OK && shared.submitted_frames == 3 &&
              shared.played_frames == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && frame(c, clip) == 3 &&
              pump_frame(&output, frame(c, clip)) == 1 && queue.total == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && frame(c, clip) == 4 &&
              pump_frame(&output, frame(c, clip)) == 0 && output.info.eof))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value visited;
        if (!(fvm_object_get(c, clip, "visited", &visited) == FVM_OK && visited.number == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        method(c, clip, "gotoAndStop", 1);
        if (!(frame(c, clip) == 1 && fvm_swf_stream_output_seek_frame(&output, 1) == FVM_OK && queue.paused))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_clock_info(composition, &shared) == FVM_OK && !shared.submitted_frames &&
              !shared.played_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pump_frame(&output, frame(c, clip)) == 3 && queue.total == 7 &&
              !std::memcmp(queue.samples, queue.samples + 8, 6 * sizeof(short))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        method(c, clip, "gotoAndStop", 3);
        if (!(fvm_swf_stream_output_seek_frame(&output, 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pump_frame(&output, frame(c, clip)) == 2 && output.info.submitted_frames == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_clock_info(composition, &shared) == FVM_OK && shared.submitted_frames == 2 &&
              shared.played_frames == 0 && shared.queued_frames == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode)
        {
            FVM_ExecutionStats stats;
            stats.struct_size = sizeof(stats);
            if (!(fvm_execution_get_stats(c, &stats) == FVM_OK && stats.native_entries > 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        ++executed;
        fvm_swf_sound_mix_queue_close(composition);
        fvm_swf_sound_mixer_close(mixer);
        if (!(fvm_gc_unpin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
        fvm_swf_stream_sound_snapshot_close(snapshot);
    }
    std::printf("PASS actual timeline authorization: interpreter=1 native_jit=%u\n", executed > 1 ? 1U : 0U);
    return 0;
}
