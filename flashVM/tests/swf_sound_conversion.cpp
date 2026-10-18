/* Rational output position, channel conversion and strict compatibility across block partitions. */
#include "swf_sound_conversion_fixture.hpp"
using namespace conversion_fixture;

static void compare(FVM_Context *context, const Samples &samples, unsigned code,
                    unsigned source_channels, unsigned output_rate, unsigned channels, size_t chunk,
                    unsigned loops)
{
    Bytes swf = movie(samples, code, source_channels, loops);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    FVM_SWFSoundMixer *mixer = create(output_rate, channels);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    Samples output(chunk * channels);
    size_t total = duration(samples, rates[code], source_channels, output_rate, loops);
    size_t done = 0, written = 99;
    assert(fvm_swf_sound_mixer_read(mixer, chunk, output.begin(), output.size() - 1, &written) ==
               FVM_INVALID_ARGUMENT &&
           !written);
    assert(fvm_swf_sound_mixer_read(mixer, 0, 0, 0, &written) == FVM_OK && !written);
    while (done < total + 3)
    {
        assert(fvm_swf_sound_mixer_read(mixer, chunk, output.begin(), output.size(), &written) ==
               FVM_OK);
        assert(written == output.size());
        for (size_t i = 0; i < chunk; ++i)
            for (unsigned ch = 0; ch < channels; ++ch)
                assert(output[i * channels + ch] == oracle(samples, rates[code], source_channels,
                                                           output_rate, channels, done + i, ch,
                                                           loops));
        done += chunk;
        assert(active(mixer) == (done < total ? 1U : 0U));
    }
    fvm_swf_sound_mixer_close(mixer);
}

int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    const unsigned output_rates[] = {5512,  5513,  8000,  11025,  22050, 44100,
                                     47999, 48000, 96000, 191999, 192000};
    const size_t chunks[] = {1, 7, 255, 256, 257, 8192};
    const short values[] = {-32768, 32767,  -1, 0,  1,     -30001, 30000,
                            12345,  -12346, 7,  -8, 20000, -20001, 29};
    for (unsigned code = 0; code < 4; ++code)
        for (unsigned source_channels = 1; source_channels <= 2; ++source_channels)
            for (unsigned channels = 1; channels <= 2; ++channels)
                for (unsigned rate = 0; rate < sizeof(output_rates) / sizeof(output_rates[0]);
                     ++rate)
                {
                    Samples samples;
                    for (unsigned i = 0; i < 7 * source_channels; ++i)
                        samples.push_back(values[i]);
                    for (unsigned part = 0; part < 6; ++part)
                        compare(context, samples, code, source_channels, output_rates[rate],
                                channels, chunks[part], 3);
                }
    /* More than a second of noninteger output ratio, across many independent small reads. */
    Samples long_source;
    for (size_t i = 0; i < 44101; ++i)
        long_source.push_back((short)((long)(i % 65536) - 32768));
    compare(context, long_source, 3, 1, 48000, 2, 7, 1);
    FVM_SWFSoundMixerConfig output;
    FVM_SWFSoundMixer *mixer = create(48000, 2);
    output.struct_size = sizeof(output);
    assert(fvm_swf_sound_mixer_get_config(mixer, &output) == FVM_OK &&
           output.flags == FVM_SWF_SOUND_MIXER_CONVERT && output.sample_rate == 48000);
    std::memset(&output, 0x7e, sizeof(output));
    FVM_SWFSoundMixerConfig before = output;
    assert(fvm_swf_sound_mixer_get_config(mixer, &output) == FVM_INVALID_ARGUMENT &&
           !std::memcmp(&output, &before, sizeof(output)));
    fvm_swf_sound_mixer_close(mixer);
    FVM_SWFSoundMixerConfig bad = {sizeof(bad), 48000, 2, 4, FVM_SWF_SOUND_MIXER_CONVERT};
    for (unsigned scenario = 0; scenario < 6; ++scenario)
    {
        FVM_SWFSoundMixerConfig candidate = bad;
        if (scenario == 0)
            candidate.struct_size = 0;
        if (scenario == 1)
            candidate.sample_rate = FVM_SWF_SOUND_MIXER_MIN_RATE - 1;
        if (scenario == 2)
            candidate.sample_rate = FVM_SWF_SOUND_MIXER_MAX_RATE + 1;
        if (scenario == 3)
            candidate.flags = 2;
        if (scenario == 4)
            candidate.channels = 3;
        if (scenario == 5)
            candidate.flags = 0; /* Strict mode still rejects non-SWF output clocks. */
        mixer = (FVM_SWFSoundMixer *)1;
        assert(fvm_swf_sound_mixer_create_configured(&candidate, &mixer) == FVM_INVALID_ARGUMENT &&
               !mixer);
    }
    assert(fvm_swf_sound_mixer_create_configured(0, &mixer) == FVM_INVALID_ARGUMENT && !mixer);
    assert(fvm_swf_sound_mixer_create_configured(&bad, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_mixer_get_config(0, &output) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_mixer_create(48000, 2, 4, &mixer) == FVM_INVALID_ARGUMENT && !mixer);
    assert(fvm_swf_sound_mixer_create(44100, 1, 4, &mixer) == FVM_OK);
    output.struct_size = sizeof(output);
    assert(fvm_swf_sound_mixer_get_config(mixer, &output) == FVM_OK && !output.flags);
    fvm_swf_sound_mixer_close(mixer);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
