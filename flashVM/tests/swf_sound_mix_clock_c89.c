/* Real C89 creation, accepted/dequeued clock, drained end and output-preserving errors. */
#include "flashvm/swf_sound_mix_queue.h"
#include <string.h>


typedef struct ClockFIFO
{
    size_t frames;
} ClockFIFO;


static FVM_Result write_frames(void *raw, const short *pcm, size_t frames)
{
    ClockFIFO *q = (ClockFIFO *)raw;
    if (!pcm && frames)
    {
        return FVM_INVALID_ARGUMENT;
    }
    q->frames += frames;
    return FVM_OK;
}


static FVM_Result count_frames(void *raw, size_t *out)
{
    *out = ((ClockFIFO *)raw)->frames;
    return FVM_OK;
}


static FVM_Result pause_frames(void *raw, int paused)
{
    (void)raw;
    (void)paused;
    return FVM_OK;
}


static FVM_Result clear_frames(void *raw)
{
    ((ClockFIFO *)raw)->frames = 0;
    return FVM_OK;
}


int main(void)
{
    FVM_SWFSoundMixer *mixer = 0;
    FVM_SWFSoundMixQueue *queue = 0;
    FVM_AudioQueueBackend b, adapted;
    FVM_SWFSoundMixQueueClock clock, saved;
    ClockFIFO fifo;
    short pcm[2] = {1, 2};
    size_t budget = 0;
    memset(&clock, 0x5a, sizeof(clock));
    clock.struct_size = sizeof(clock);
    saved = clock;
    if (fvm_swf_sound_mix_queue_clock_begin(0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_sound_mix_queue_clock_end(0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_sound_mix_queue_clock_info(0, &clock) != FVM_INVALID_ARGUMENT || memcmp(&clock, &saved, sizeof(clock)))
    {
        return 1;
    }
    memset(&b, 0, sizeof(b));
    fifo.frames = 0;
    b.struct_size = sizeof(b);
    b.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    b.sample_rate = 44100;
    b.channels = 1;
    b.userdata = &fifo;
    b.write = write_frames;
    b.queued = count_frames;
    b.pause = pause_frames;
    b.clear = clear_frames;
    if (fvm_swf_sound_mixer_create(44100, 1, 1, &mixer) != FVM_OK ||
        fvm_swf_sound_mix_queue_storage_required(1, 2, &budget) != FVM_OK ||
        fvm_swf_sound_mix_queue_create_bounded(mixer, &b, 2, budget, &queue, &adapted) != FVM_OK)
    {
        return 1;
    }
    if (fvm_swf_sound_mix_queue_clock_info(queue, &clock) != FVM_NOT_FOUND || memcmp(&clock, &saved, sizeof(clock)) ||
        fvm_swf_sound_mix_queue_clock_begin(queue) != FVM_OK)
    {
        return 1;
    }
    if (adapted.write(adapted.userdata, pcm, 2) != FVM_OK ||
        fvm_swf_sound_mix_queue_clock_info(queue, &clock) != FVM_OK || clock.submitted_frames != 2 ||
        clock.queued_frames != 2 || clock.played_frames != 0)
    {
        return 1;
    }
    fifo.frames = 1;
    if (fvm_swf_sound_mix_queue_clock_info(queue, &clock) != FVM_OK || clock.played_frames != 1 ||
        clock.seconds != 1.0 / 44100 || fvm_swf_sound_mix_queue_clock_end(queue) != FVM_NOT_FOUND)
    {
        return 1;
    }
    saved = clock;
    clock.struct_size = 0;
    saved.struct_size = 0;
    if (fvm_swf_sound_mix_queue_clock_info(queue, &clock) != FVM_INVALID_ARGUMENT ||
        memcmp(&clock, &saved, sizeof(clock)))
    {
        return 1;
    }
    clock.struct_size = sizeof(clock);
    fifo.frames = 0;
    if (fvm_swf_sound_mix_queue_clock_end(queue) != FVM_OK)
    {
        return 1;
    }
    saved = clock;
    if (fvm_swf_sound_mix_queue_clock_info(queue, &clock) != FVM_NOT_FOUND || memcmp(&clock, &saved, sizeof(clock)))
    {
        return 1;
    }
    fvm_swf_sound_mix_queue_close(queue);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
