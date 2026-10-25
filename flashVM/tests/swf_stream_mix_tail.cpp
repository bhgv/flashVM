/* Paired EOF tails preserve both queue clocks and exact event retry after VM teardown. */
#define FVM_MIX_IDENTITY_STREAM_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_stream.cpp"


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
    FVM_SWFStreamSoundSnapshot *source = 0;
    if (!(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *voice[2] = {0, 0};
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_swf_sound_voice_open(c, i, voice + i) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    FVM_SWFSoundMixerConfig geometry = {sizeof(geometry), 48000, 2, 2, FVM_SWF_SOUND_MIXER_CONVERT};
    FVM_SWFSoundMixer *mixer = 0;
    if (!(fvm_swf_sound_mixer_create_configured(&geometry, &mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Queue q;
    std::memset(&q, 0, sizeof(q));
    FVM_AudioQueueBackend b;
    std::memset(&b, 0, sizeof(b));
    b.struct_size = sizeof(b);
    b.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    b.sample_rate = 48000;
    b.channels = 2;
    b.userdata = &q;
    b.write = write_pcm;
    b.queued = queued;
    b.pause = pause_native;
    b.clear = clear_native;
    FVM_AudioQueueBackend adapter;
    FVM_SWFSoundMixQueue *mix = 0;
    size_t bytes = 0;
    if (!(fvm_swf_sound_mix_queue_create(mixer, &b, 2, &mix, &adapter) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.adapter = &adapter;
    if (!(fvm_swf_sound_mix_queue_clock_begin(mix) == FVM_OK &&
          fvm_swf_sound_mix_queue_storage_required_for_tracking(2, 2, 1, &bytes) == FVM_OK &&
          fvm_swf_sound_mix_queue_set_identity_tracking(mix, 1, bytes) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_swf_sound_mix_queue_submit_prepared_bounded(mix, voice + i, (size_t)-1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }

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
    size_t n = 91;
    if (!(fvm_swf_stream_output_pump_mix_tail(&output, mix, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned guard = 0; guard < 16; ++guard)
    {
        q.count = 0;
        FVM_Result r = fvm_swf_stream_output_pump(&output, &n);
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
    if (!(q.accepted == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamConverter cursor = converter;
    q.fail = 1;
    if (!(fvm_swf_stream_output_pump_mix_tail(&output, mix, &n) == FVM_ERROR && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueuePending pending;
    pending.struct_size = sizeof(pending);
    if (!(fvm_swf_sound_mix_queue_pending(mix, &pending) == FVM_OK && pending.frames == 2 &&
          pending.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail = 0;
    for (unsigned guard = 0; guard < 8; ++guard)
    {
        q.count = 0;
        FVM_Result r = fvm_swf_stream_output_pump_mix_tail(&output, mix, &n);
        if (r == FVM_NOT_FOUND)
        {
            break;
        }
        if (!(r == FVM_OK && n == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(q.accepted == 15 && !std::memcmp(&cursor, &converter, sizeof(cursor))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const short expected[] = {
        30000, -25125, 11875, -10250, 250, -1875, 10500, 4125, -30000, -30000, -30000, -30000, -30000, -30000, 0};
    for (size_t i = 0; i < 15; ++i)
    {
        if (!(q.saved[i * 2] == expected[i] && q.saved[i * 2 + 1] == expected[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_SWFStreamOutputInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_stream_output_info(&output, &info) == FVM_OK && info.submitted_frames == 15 &&
          info.played_frames == 15 && info.eof))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueClock clock;
    clock.struct_size = sizeof(clock);
    if (!(fvm_swf_sound_mix_queue_clock_info(mix, &clock) == FVM_OK &&
          clock.submitted_frames == info.submitted_frames && clock.played_frames == info.played_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek(&output, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_info(&output, &info) == FVM_OK && !info.submitted_frames && !info.eof))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(mix);
    fvm_swf_sound_mixer_close(mixer);
    fvm_swf_stream_sound_snapshot_close(source);
    return 0;
}
