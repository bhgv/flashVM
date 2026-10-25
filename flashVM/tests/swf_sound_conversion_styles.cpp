/* Decode/style on the source clock first, then compare conversion with an absolute-time oracle. */
#define main voice_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_voice.cpp"

#undef main
#include "swf_sound_conversion_fixture.hpp"


static void check(FVM_Context *context, unsigned source_channels, unsigned output_channels, size_t chunk)
{
    FVM_SWFSoundVoice *voice = 0;
    if (!(fvm_swf_sound_voice_open(context, 0, &voice) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    conversion_fixture::Samples styled, buffer(257 * source_channels);
    for (;;)
    {
        size_t count = 0;
        FVM_Result result = fvm_swf_sound_voice_read(voice, 257, buffer.begin(), buffer.size(), &count);
        if (result == FVM_NOT_FOUND)
        {
            break;
        }
        if (!(result == FVM_OK && count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < count; ++i)
        {
            styled.push_back(buffer[i]);
        }
    }
    fvm_swf_sound_voice_close(voice);
    FVM_SWFSoundMixer *mixer = conversion_fixture::create(48000, output_channels);
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    conversion_fixture::Samples output(chunk * output_channels);
    size_t total = conversion_fixture::duration(styled, 44100, source_channels, 48000);
    for (size_t at = 0; at < total + 3; at += chunk)
    {
        size_t written = 0;
        if (!(fvm_swf_sound_mixer_read(mixer, chunk, output.begin(), output.size(), &written) == FVM_OK &&
              written == output.size()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < chunk; ++i)
        {
            for (unsigned ch = 0; ch < output_channels; ++ch)
            {
                if (!(output[i * output_channels + ch] ==
                      conversion_fixture::oracle(styled, 44100, source_channels, 48000, output_channels, at + i, ch)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    if (!(!conversion_fixture::active(mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(mixer);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const size_t chunks[] = {1, 255, 256, 257, 8192};
    for (unsigned source_channels = 1; source_channels <= 2; ++source_channels)
    {
        for (unsigned width = 2; width <= 5; ++width)
        {
            Bits bits;
            bits.put(width - 2, 2);
            for (unsigned frame = 0; frame < 4098; ++frame)
            {
                for (unsigned ch = 0; ch < source_channels; ++ch)
                {
                    if (frame % 4096 == 0)
                    {
                        bits.put(ch ? 33536 : 32000, 16);
                        bits.put(63, 6);
                    }
                    else
                    {
                        bits.put((frame * 13 + ch * 7) % (1U << width), width);
                    }
                }
            }
            Bytes swf = file(0x1e | (source_channels - 1), 4098, bits.b, 8);
            if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            compare(context, source_channels); /* Existing complete-event rendering cross-check. */
            for (unsigned output_channels = 1; output_channels <= 2; ++output_channels)
            {
                for (unsigned i = 0; i < 5; ++i)
                {
                    check(context, source_channels, output_channels, chunks[i]);
                }
            }
        }
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
