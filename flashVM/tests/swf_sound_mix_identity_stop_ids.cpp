/* One global control cancels distinct root IDs after converted composition and VM teardown. */
#define main stop_ids_tutorial_main
#include <stdio.h>
#include <stdlib.h>
#include "../examples/multimedia/swf_stream_mix/main.c"

#undef main


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *voice[2] = {0, 0}, *again[2] = {0, 0};
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_swf_sound_voice_open(c, i, voice + i) == FVM_OK &&
              fvm_swf_sound_voice_clone(voice[i], (size_t)-1, again + i) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    FVM_SWFSoundMixerConfig geometry = {sizeof(geometry), 48000, 2, 4, FVM_SWF_SOUND_MIXER_CONVERT};
    FVM_SWFSoundMixer *mixer = 0;
    if (!(fvm_swf_sound_mixer_create_configured(&geometry, &mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ExampleQueue fifo;
    memset(&fifo, 0, sizeof(fifo));
    FVM_AudioQueueBackend b;
    memset(&b, 0, sizeof(b));
    b.struct_size = sizeof(b);
    b.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    b.sample_rate = 48000;
    b.channels = 2;
    b.userdata = &fifo;
    b.write = queue_write;
    b.queued = queue_count;
    b.pause = queue_pause;
    b.clear = queue_clear;
    FVM_AudioQueueBackend adapted;
    FVM_SWFSoundMixQueue *q = 0;
    size_t bytes = 0;
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(2, 2, 1, &bytes) == FVM_OK &&
          fvm_swf_sound_mix_queue_create(mixer, &b, 2, &q, &adapted) == FVM_OK &&
          fvm_swf_sound_mix_queue_clock_begin(q) == FVM_OK &&
          fvm_swf_sound_mix_queue_set_identity_tracking(q, 1, bytes) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(q, voice + i, (size_t)-1) == FVM_OK && !voice[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    size_t n = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(q, 2, 8, &n) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_stop_all(q) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned ids[] = {1, 7, 8, 65535};
    for (unsigned i = 0; i < 4; ++i)
    {
        FVM_SWFSoundMixQueueIdentityInfo info;
        info.struct_size = sizeof(info);
        if (!(fvm_swf_sound_mix_queue_identity_info(q, ids[i], &info) == FVM_OK && !info.end_frame && !info.active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(q, again + i, (size_t)-1) == FVM_OK && !again[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_SWFSoundMixQueueIdentityInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_identity_info(q, 7, &info) == FVM_OK && info.end_frame == 11))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_identity_info(q, 8, &info) == FVM_OK && info.end_frame == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_stop_all(q) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(q);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
