/* Opt-in stop-by-ID fade: exact direct/converted endpoints, split reads and legacy hard stops. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_conversion_fixture.hpp"

using namespace conversion_fixture;


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
    Samples samples;
    for (unsigned i = 0; i < 5000; ++i)
    {
        samples.push_back(10000);
    }
    Bytes swf = movie(samples, 3, 1);
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned variant = 0; variant < 4; ++variant)
    {
        unsigned rate = variant & 1 ? 48000 : 44100, channels = variant & 1 ? 2 : 1;
        bool smooth = variant >= 2;
        FVM_SWFSoundMixerConfig config = {sizeof(config),
                                          rate,
                                          channels,
                                          2,
                                          FVM_SWF_SOUND_MIXER_CONVERT | (smooth ? FVM_SWF_SOUND_MIXER_DECLICK : 0)};
        FVM_SWFSoundMixer *mixer = 0;
        if (!(fvm_swf_sound_mixer_create_configured(&config, &mixer) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        short pcm[32];
        size_t written = 0;
        if (!(fvm_swf_sound_mixer_read(mixer, 7, pcm, 32, &written) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mixer_submit(mixer, context, 2) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!smooth)
        {
            if (!(active(mixer) == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_mixer_read(mixer, 7, pcm, 32, &written) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (size_t i = 0; i < written; ++i)
            {
                if (!(!pcm[i]))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        else
        {
            size_t total = rate / 200, at = 0;
            short previous = 10000;
            while (at < total)
            {
                size_t count = total - at < 7 ? total - at : 7;
                if (!(fvm_swf_sound_mixer_read(mixer, count, pcm, 32, &written) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (size_t f = 0; f < count; ++f)
                {
                    short expected = (short)(10000.0 * (total - 1 - at - f) / (total - 1));
                    for (unsigned c = 0; c < channels; ++c)
                    {
                        if (!(pcm[f * channels + c] == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(pcm[f * channels] <= previous && previous - pcm[f * channels] < 50))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    previous = pcm[f * channels];
                }
                at += count;
            }
            if (!(previous == 0 && active(mixer) == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mixer_submit(mixer, context, 2) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (smooth)
        {
            if (!(active(mixer) == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_mixer_submit(mixer, context, 1) == FVM_OK && active(mixer) == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK && active(mixer) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_sound_mixer_close(mixer);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
}
