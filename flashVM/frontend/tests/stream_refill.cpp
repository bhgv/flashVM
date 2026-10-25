/* Deterministic consumption and backend failures exercise the actual C89 worker refill. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/audio.h"
#include "../../tests/swf_sound_fixture.hpp"
#include <climits>
#include <cstring>

using namespace sound_fixture;
extern "C" FVM_Result fvm_test_stream_refill(FVM_DesktopAudio *);


struct Queue
{
    size_t count, pauses, writes;
    int paused, fail_pause, fail_write, fail_resume;
};


static FVM_Result write_pcm(void *p, const short *pcm, size_t frames)
{
    Queue *q = (Queue *)p;
    ++q->writes;
    for (size_t i = 0; i < frames * 2; ++i)
    {
        if (!(pcm[i] == 100))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (q->fail_write)
    {
        return FVM_ERROR;
    }
    q->count += frames;
    return FVM_OK;
}


static FVM_Result queued(void *p, size_t *out)
{
    *out = ((Queue *)p)->count;
    return FVM_OK;
}


static FVM_Result pause_queue(void *p, int paused)
{
    Queue *q = (Queue *)p;
    ++q->pauses;
    if (q->fail_pause || (!paused && q->fail_resume))
    {
        return FVM_ERROR;
    }
    q->paused = paused;
    return FVM_OK;
}


static FVM_Result clear_queue(void *p)
{
    ((Queue *)p)->count = 0;
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
    Bytes tags, head, pcm;
    head.push_back(0x0e);
    head.push_back(0x3e);
    word(head, 1);
    tag(tags, 45, head);
    for (size_t i = 0; i < 30000; ++i)
    {
        word(pcm, 100);
    }
    tag(tags, 19, pcm);
    Bytes swf = movie(tags);
    FVM_DesktopAudio a;
    FVM_DesktopStreamSegment segment;
    std::memset(&a, 0, sizeof(a));
    std::memset(&segment, 0, sizeof(segment));
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
          fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &segment.source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_converter_init(&segment.converter, segment.source, 48000, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    a.stream_source = segment.source;
    a.stream_segments = &segment;
    a.stream_count = 1;
    FVM_SDLAudioQueueInfo device;
    std::memset(&device, 0, sizeof(device));
    device.struct_size = sizeof(device);
    device.sample_rate = 48000;
    device.channels = 2;
    a.trace = fvm_desktop_trace_open(&device, 4800, 2048);
    if (SDL_getenv("FVM_AUDIO_TRACE"))
    {
        if (!(a.trace))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    a.stream_frame = 1;
    a.resume_frames = 2048;
    Queue q;
    std::memset(&q, 0, sizeof(q));
    FVM_AudioQueueBackend backend;
    std::memset(&backend, 0, sizeof(backend));
    backend.struct_size = sizeof(backend);
    backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    backend.userdata = &q;
    backend.sample_rate = 48000;
    backend.channels = 2;
    backend.write = write_pcm;
    backend.queued = queued;
    backend.pause = pause_queue;
    backend.clear = clear_queue;
    if (!(fvm_swf_stream_output_init(&a.stream_output,
                                     &segment.converter,
                                     &backend,
                                     a.stream_pcm,
                                     sizeof(a.stream_pcm) / sizeof(short),
                                     1024,
                                     4800) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* First priming is not starvation; the actual worker fills and starts the queue. */
    if (!(fvm_test_stream_refill(&a) == FVM_OK && q.count == 4800 && !q.paused && SDL_AtomicGet(&a.underruns) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 0;
    q.fail_pause = 1;
    size_t writes = q.writes;
    if (!(fvm_test_stream_refill(&a) == FVM_ERROR && q.writes == writes && !a.was_starved &&
          SDL_AtomicGet(&a.underruns) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Pause succeeds, write rejects: one edge, exact pending retry, no duplicate count. */
    q.fail_pause = 0;
    q.fail_write = 1;
    if (!(fvm_test_stream_refill(&a) == FVM_ERROR && q.paused && !q.count && a.was_starved &&
          SDL_AtomicGet(&a.underruns) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_test_stream_refill(&a) == FVM_ERROR && SDL_AtomicGet(&a.underruns) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail_write = 0;
    q.fail_resume = 1;
    if (!(fvm_test_stream_refill(&a) == FVM_ERROR && q.count == 4800 && q.paused && a.was_starved &&
          SDL_AtomicGet(&a.underruns) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail_resume = 0;
    if (!(fvm_test_stream_refill(&a) == FVM_OK && q.count == 4800 && !q.paused && !a.was_starved &&
          SDL_AtomicGet(&a.underruns) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 0;
    if (!(fvm_test_stream_refill(&a) == FVM_OK && SDL_AtomicGet(&a.underruns) == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A future-frame gate is intentional idle, even after real accepted data drained. */
    q.count = 0;
    a.stream_frame = 0;
    writes = q.writes;
    if (!(fvm_test_stream_refill(&a) == FVM_OK && q.writes == writes && SDL_AtomicGet(&a.underruns) == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    a.stream_frame = 1;
    if (!(fvm_test_stream_refill(&a) == FVM_OK && SDL_AtomicGet(&a.underruns) == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 0;
    SDL_AtomicSet(&a.underruns, INT_MAX - 1);
    if (!(fvm_test_stream_refill(&a) == FVM_OK && SDL_AtomicGet(&a.underruns) == INT_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 0;
    if (!(fvm_test_stream_refill(&a) == FVM_OK && SDL_AtomicGet(&a.underruns) == INT_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A final short authorized suffix must drain even below the normal resume reserve. */
    FVM_SWFStreamOutputInfo info;
    info.struct_size = sizeof(info);
    bool tail = false;
    a.resume_frames = 4800;
    for (unsigned step = 0; step < 20; ++step)
    {
        q.count = 0;
        if (!(fvm_test_stream_refill(&a) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_output_info(&a.stream_output, &info) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (info.eof)
        {
            tail = q.count > 0 && q.count < a.resume_frames && !q.paused;
            break;
        }
    }
    if (!(tail))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.count = 0;
    size_t pauses = q.pauses;
    if (!(fvm_test_stream_refill(&a) == FVM_OK && q.pauses == pauses && !a.was_starved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Native StopSounds/movie stop do not create a starvation edge. */
    q.count = 0;
    a.stream_movie_paused = 1;
    if (!(fvm_test_stream_refill(&a) == FVM_OK && q.paused && !a.was_starved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    a.stream_movie_paused = 0;
    a.stream_cancelled = 1;
    if (!(fvm_test_stream_refill(&a) == FVM_OK && !a.was_starved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    a.stream_cancelled = 0;
    if (!(fvm_swf_stream_output_seek(&a.stream_output, segment.converter.block_count, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pause(&a.stream_output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_test_stream_refill(&a) == FVM_OK && !a.was_starved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_trace_close(a.trace);
    fvm_swf_stream_sound_snapshot_close(segment.source);
    return 0;
}
