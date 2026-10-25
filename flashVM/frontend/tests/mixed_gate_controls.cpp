/* Root stop cannot reinterpret retained BASE; acknowledged StopSounds cancels it before a new event. */
#include <stdio.h>
#include <stdlib.h>
#include "mixed_native.hpp"


int main()
{
    MixedNative f;
    f.advance();
    size_t n = 0;
    if (!(fvm_swf_stream_output_pump_mix_until(&f.audio.stream_output, f.audio.composition, 1, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_write = 1;
    if (!(fvm_test_mixed_refill(&f.audio) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueuePending pending;
    pending.struct_size = sizeof(pending);
    if (!(fvm_swf_sound_mix_queue_pending(f.audio.composition, &pending) == FVM_OK &&
          pending.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamConverter cursor = f.audio.stream_segments[0].converter;
    FVM_Value out;
    if (!(fvm_object_call(f.context, f.clip, "stop", 0, 0, &out) == FVM_OK &&
          fvm_desktop_audio_timeline(f.context, f.clip, &f.audio) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_write = 0;
    size_t calls = f.fifo.calls;
    f.refill();
    if (!(f.fifo.calls == calls && !memcmp(&cursor, &f.audio.stream_segments[0].converter, sizeof(cursor))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int has = 0;
    if (!(fvm_desktop_audio_stream_pending(&f.audio, &has) == FVM_OK && has))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 0;
    if (!(fvm_desktop_audio_stream_pending(&f.audio, &has) == FVM_OK && !has))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(f.context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_clear = 1;
    if (!(fvm_audio_stop_all(f.context) == FVM_ERROR && !f.audio.stops))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pending(f.audio.composition, &pending) == FVM_OK && pending.frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_clear = 0;
    if (!(fvm_audio_stop_all(f.context) == FVM_OK && f.audio.stops == 1 && !active(f.audio.mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value frame;
    fvm_value_number(&frame, 2);
    if (!(fvm_object_call(f.context, f.clip, "gotoAndStop", 1, &frame, &out) == FVM_OK &&
          fvm_desktop_audio_timeline(f.context, f.clip, &f.audio) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.advance();
    if (!(f.audio.stream_movie_paused && f.audio.events == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 8; ++i)
    {
        f.fifo.count = 0;
        f.refill();
    }
    if (!(f.fifo.total == 12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(f.fifo.samples[i * 2] == 2100))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 2; i < 12; ++i)
    {
        if (!(f.fifo.samples[i * 2] == (i < 11 ? 2000 : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
