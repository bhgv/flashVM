/* Event stop preserves real converted base-stream progression and a rejected mixed retry. */
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
    for (unsigned i = 0; i < 1; ++i)
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
    for (unsigned i = 0; i < 1; ++i)
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
    if (!(fvm_swf_stream_output_init(&output, &converter, &adapter, retry, 4, 2, 16) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t n = 0;
    q.fail = 1;
    if (!(fvm_swf_stream_output_pump(&output, &n) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamConverter cursor = converter;
    if (!(fvm_swf_sound_mix_queue_stop_all(mix) == FVM_OK && !std::memcmp(&cursor, &converter, sizeof(cursor))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail = 0;
    if (!(fvm_swf_stream_output_pump(&output, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned guard = 0; guard < 16; ++guard)
    {
        q.count = 0;
        FVM_Result result = fvm_swf_stream_output_pump(&output, &n);
        if (result == FVM_NOT_FOUND)
        {
            break;
        }
        if (!(result == FVM_OK))
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
    /* Independent absolute-grid rational oracle: first retry keeps its mixture; later frames are base only. */
    const long base[] = {30000, -30000, 20000, -20000, 10000, -10000, 30000, -30000};
    for (size_t frame = 0; frame < 9; ++frame)
    {
        size_t at = frame * 44100 / 48000;
        long phase = (long)(frame * 44100 % 48000);
        long value = (base[at] * (48000 - phase) + base[at < 7 ? at + 1 : at] * phase) / 48000;
        if (frame < 2)
        {
            value += 30000;
        }
        if (value > 32767)
        {
            value = 32767;
        }
        if (!(q.saved[frame * 2] == value && q.saved[frame * 2 + 1] == value))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_SWFSoundMixQueueIdentityInfo id;
    id.struct_size = sizeof(id);
    if (!(fvm_swf_sound_mix_queue_identity_info(mix, 7, &id) == FVM_OK && !id.end_frame &&
          fvm_swf_sound_mix_queue_identity_info(mix, 8, &id) == FVM_OK && !id.end_frame))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueClock clock;
    clock.struct_size = sizeof(clock);
    if (!(fvm_swf_sound_mix_queue_clock_info(mix, &clock) == FVM_OK && clock.submitted_frames == 9 &&
          clock.played_frames == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail(mix, 2, 16, &n) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(mix);
    fvm_swf_sound_mixer_close(mixer);
    fvm_swf_stream_sound_snapshot_close(source);
    return 0;
}
