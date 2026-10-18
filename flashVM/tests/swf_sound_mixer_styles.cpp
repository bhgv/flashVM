/* Existing independent full-style oracle plus PCM/ADPCM fixtures exercise mixer chunk boundaries.
 */
#define main voice_style_fixture_main
#include "swf_sound_voice.cpp"
#undef main
#include "flashvm/swf_sound_mixer.h"

static void check_mixer(FVM_Context *context, unsigned channels, size_t chunk)
{
    FVM_SWFSoundVoice *reference = 0;
    FVM_SWFSoundMixer *mixer = 0;
    assert(fvm_swf_sound_voice_open(context, 0, &reference) == FVM_OK);
    assert(fvm_swf_sound_mixer_create(44100, channels, 2, &mixer) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    fvm_detail::vector<short> a(chunk * channels), b(chunk * channels);
    for (;;)
    {
        size_t received = 0, produced = 0;
        FVM_Result result =
            fvm_swf_sound_voice_read(reference, chunk, a.begin(), a.size(), &received);
        assert(fvm_swf_sound_mixer_read(mixer, chunk, b.begin(), b.size(), &produced) == FVM_OK);
        assert(produced == chunk * channels);
        for (size_t i = 0; i < produced; ++i)
        {
            long expected = i < received ? (long)a[i] * 2 : 0;
            if (expected < -32768)
                expected = -32768;
            if (expected > 32767)
                expected = 32767;
            assert(b[i] == expected);
        }
        if (result == FVM_NOT_FOUND)
        {
            assert(!received);
            break;
        }
        assert(result == FVM_OK && received);
    }
    fvm_swf_sound_voice_close(reference);
    fvm_swf_sound_mixer_close(mixer);
}

int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    const size_t chunks[] = {1, 7, 255, 256, 257, 8192};
    for (unsigned channels = 1; channels <= 2; ++channels)
        for (unsigned width = 2; width <= 5; ++width)
        {
            Bits bits;
            bits.put(width - 2, 2);
            for (unsigned frame = 0; frame < 4098; ++frame)
                for (unsigned ch = 0; ch < channels; ++ch)
                    if (frame % 4096 == 0)
                    {
                        bits.put(ch ? 33536 : 32000, 16);
                        bits.put(63, 6);
                    }
                    else
                        bits.put((frame * 13 + ch * 7) % (1U << width), width);
            Bytes swf = file(0x1e | (channels - 1), 4098, bits.b, 8);
            assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
            compare(context,
                    channels); /* Cross-check the reference against complete event rendering. */
            for (unsigned i = 0; i < 6; ++i)
                check_mixer(context, channels, chunks[i]);
        }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
