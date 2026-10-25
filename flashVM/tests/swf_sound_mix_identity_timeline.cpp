/* Actual root frame callbacks adopt/stop tracked identities in interpreter and native JIT modes. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_sound.cpp"
#include "swf_sound_output_fixture.hpp"
#include "flashvm/swf_sound_mix_queue.h"


struct TrackedDelivery
{
    FVM_SWFSoundMixQueue *queue;
    unsigned count;
};


static FVM_Result tracked_receive(FVM_Context *c, FVM_Object *clip, size_t ordinal, void *raw)
{
    TrackedDelivery *d = (TrackedDelivery *)raw;
    if (!(fvm_movieclip_advance(c, clip, 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundEvent event;
    event.struct_size = sizeof(event);
    FVM_Result r = fvm_swf_sound_event_at(c, ordinal, &event);
    if (r != FVM_OK)
    {
        return r;
    }
    if (event.flags & FVM_SWF_SOUND_EVENT_STOP)
    {
        r = fvm_swf_sound_mix_queue_stop_id(d->queue, event.sound_id);
    }
    else
    {
        FVM_SWFSoundVoice *voice = 0;
        r = fvm_swf_sound_voice_open_bounded(c, ordinal, (size_t)-1, &voice);
        if (r == FVM_OK)
        {
            r = fvm_swf_sound_mix_queue_submit_prepared_bounded(d->queue, &voice, (size_t)-1);
        }
        fvm_swf_sound_voice_close(voice);
    }
    if (r == FVM_OK)
    {
        ++d->count;
    }
    return r;
}


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
              fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes swf = sound_movie();
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *clip = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &clip) == FVM_OK &&
              fvm_gc_pin(c, clip) == FVM_OK &&
              fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixer *mixer = conversion_fixture::create(48000, 2);
        output_fixture::Device device;
        FVM_AudioQueueBackend real = output_fixture::backend(device), adapted;
        FVM_SWFSoundMixQueue *queue = 0;
        size_t bytes = 0;
        if (!(fvm_swf_sound_mix_queue_create(mixer, &real, 4, &queue, &adapted) == FVM_OK &&
              fvm_swf_sound_mix_queue_clock_begin(queue) == FVM_OK &&
              fvm_swf_sound_mix_queue_storage_required(2, 4, &bytes) == FVM_OK &&
              fvm_swf_sound_mix_queue_set_identity_tracking(
                  queue, 1, bytes + FVM_SWF_SOUND_MIX_QUEUE_ID_SLOTS * sizeof(double)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        TrackedDelivery delivery = {queue, 0};
        if (!(fvm_movieclip_set_sound_callback(c, tracked_receive, &delivery) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && delivery.count == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t accepted = 0;
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 4, 16, &accepted) == FVM_OK && accepted == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixQueueIdentityInfo info;
        info.struct_size = sizeof(info);
        if (!(fvm_swf_sound_mix_queue_identity_info(queue, 7, &info) == FVM_OK && info.end_frame == 3 && info.active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, clip, 2) == FVM_OK && delivery.count == 2 && fvm_gc_step(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        seek(c, clip, 3, true);
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && delivery.count == 3 &&
              fvm_swf_sound_mix_queue_identity_info(queue, 7, &info) == FVM_OK && !info.end_frame))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        seek(c, clip, 4, true);
        if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK && delivery.count == 4 &&
              fvm_swf_sound_mix_queue_identity_info(queue, 7, &info) == FVM_OK && info.end_frame == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_ExecutionStats stats;
        stats.struct_size = sizeof(stats);
        if (!(fvm_execution_get_stats(c, &stats) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode)
        {
            if (!(stats.native_entries > 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(stats.interpreter_entries > 0 && !stats.native_entries))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_gc_unpin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
        if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 4, 16, &accepted) == FVM_OK && accepted == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const short expected[] = {-32768, -32768, 27208, 27208, 32512, 32512, 0, 0};
        if (!(device.values == 16 && !std::memcmp(device.history, expected, sizeof(expected)) &&
              !std::memcmp(device.history + 8, expected, sizeof(expected))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(adapted.pause(adapted.userdata, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        output_fixture::consume(device, 8);
        if (!(fvm_swf_sound_mix_queue_identity_info(queue, 7, &info) == FVM_OK && !info.active &&
              info.played_frames == 8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mix_queue_set_identity_tracking(queue, 0, 0) == FVM_OK &&
              fvm_swf_sound_mix_queue_clock_end(queue) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_sound_mix_queue_close(queue);
        fvm_swf_sound_mixer_close(mixer);
        ++executed;
    }
    std::printf("PASS tracked root sound callback: interpreter=1 native_jit=%u\n", executed > 1 ? 1U : 0U);
    return 0;
}
