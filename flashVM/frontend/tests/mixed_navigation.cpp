/* Mixed navigation stages clear before new frame events; pause and retained tail work stay explicit. */
#include <stdio.h>
#include <stdlib.h>
#include "mixed_native.hpp"


static void go(MixedNative &f, unsigned frame)
{
    FVM_Value arg, out;
    fvm_value_number(&arg, frame);
    if (!(fvm_object_call(f.context, f.clip, "gotoAndPlay", 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    {
        MixedNative f;
        f.advance();
        size_t n = 0;
        for (unsigned i = 0; i < 8; ++i)
        {
            f.fifo.count = 0;
            FVM_Result r = fvm_swf_stream_output_pump_until(&f.audio.stream_output, 1, &n);
            if (r == FVM_NOT_FOUND)
            {
                break;
            }
            if (!(r == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(f.fifo.total == 5))
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
        FVM_SWFSoundMixQueuePending tail;
        tail.struct_size = sizeof(tail);
        if (!(fvm_swf_sound_mix_queue_pending(f.audio.composition, &tail) == FVM_OK &&
              tail.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_stop_all(f.audio.composition) == FVM_OK && !active(f.audio.mixer)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int pending = 0;
        if (!(fvm_desktop_audio_stream_pending(&f.audio, &pending) == FVM_OK && pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamConverter cursor = f.audio.stream_segments[0].converter;
        if (!(fvm_desktop_audio_pause(&f.audio, 1) == FVM_OK && f.fifo.paused))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_audio_stream_pending(&f.audio, &pending) == FVM_OK && !pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::memcmp(&cursor, &f.audio.stream_segments[0].converter, sizeof(cursor))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_audio_pause(&f.audio, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_write = 0;
        f.refill();
        if (!(f.fifo.total == 7 && f.fifo.samples[10] == 2000 && f.fifo.samples[12] == 2000))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.count = 0;
        if (!(fvm_desktop_audio_stream_pending(&f.audio, &pending) == FVM_OK && !pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    {
        MixedNative f;
        f.advance();
        f.fifo.fail_write = 1;
        if (!(fvm_test_mixed_refill(&f.audio) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        go(f, 1);
        f.fifo.fail_clear = 1;
        if (!(fvm_desktop_audio_timeline(f.context, f.clip, &f.audio) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueuePending pending;
        pending.struct_size = sizeof(pending);
        if (!(fvm_swf_sound_mix_queue_pending(f.audio.composition, &pending) == FVM_OK && pending.frames == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueueIdentityInfo id;
        id.struct_size = sizeof(id);
        if (!(fvm_swf_sound_mix_queue_identity_info(f.audio.composition, 7, &id) == FVM_OK && id.end_frame == 9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_clear = 0;
        if (!(fvm_desktop_audio_timeline(f.context, f.clip, &f.audio) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_pending(f.audio.composition, &pending) == FVM_OK && !pending.frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_write = 0;
        f.advance();
        if (!(f.audio.events == 2 && active(f.audio.mixer) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_identity_info(f.audio.composition, 7, &id) == FVM_OK && id.end_frame == 9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.refill();
        if (!(f.fifo.total == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < 8; ++i)
        {
            if (!(f.fifo.samples[i] == 2100))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}
