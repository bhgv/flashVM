/* Base PCM participates in the single clipping pass, across native geometry and chunk boundaries.
 */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_conversion_fixture.hpp"
#include <cstring>

using namespace conversion_fixture;


int main()
{
    for (unsigned r = 0; r < 4; ++r)
    {
        for (unsigned channels = 1; channels <= 2; ++channels)
        {
            for (unsigned converted = 0; converted < 2; ++converted)
            {
                FVM_Config cfg;
                FVM_Runtime *rt = 0;
                FVM_Context *c = 0;
                if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
                      fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Samples source;
                for (unsigned i = 0; i < 600; ++i)
                {
                    for (unsigned ch = 0; ch < channels; ++ch)
                    {
                        source.push_back(ch ? -30000 : 30000);
                    }
                }
                Bytes swf = movie(source, r, channels);
                if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned rate = converted ? 48000 : rates[r];
                FVM_SWFSoundMixer *mixer = 0;
                if (converted)
                {
                    mixer = create(rate, channels);
                }
                else
                {
                    if (!(fvm_swf_sound_mixer_create(rate, channels, 2, &mixer) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK &&
                      fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
                fvm_runtime_destroy(rt);
                size_t end = duration(source, rates[r], channels, rate);
                Samples pcm;
                pcm.resize((end + 3) * channels);
                for (size_t i = 0; i < pcm.size(); ++i)
                {
                    pcm[i] = i % channels ? 30000 : -30000;
                }
                size_t written = 91;
                if (!(fvm_swf_sound_mixer_add(mixer, end + 3, pcm.begin(), pcm.size() - 1, &written) ==
                          FVM_INVALID_ARGUMENT &&
                      !written && active(mixer) == 2 && pcm[0] == -30000))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_mixer_add(mixer, (size_t)-1, pcm.begin(), 1, &written) == FVM_INVALID_ARGUMENT &&
                      !written && active(mixer) == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_mixer_add(mixer, 0, 0, 0, &written) == FVM_OK && !written))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_mixer_add(mixer, 1, 0, 1, &written) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_mixer_add(mixer, 1, pcm.begin(), pcm.size(), 0) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                size_t at = 0;
                while (at < end + 3)
                {
                    size_t frames = at ? 513 : 1;
                    if (frames > end + 3 - at)
                    {
                        frames = end + 3 - at;
                    }
                    FVM_Result result =
                        at == 1 ? fvm_swf_sound_mixer_read(
                                      mixer, frames, pcm.begin() + at * channels, pcm.size() - at * channels, &written)
                                : fvm_swf_sound_mixer_add(
                                      mixer, frames, pcm.begin() + at * channels, pcm.size() - at * channels, &written);
                    if (!(result == FVM_OK && written == frames * channels))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    at += frames;
                }
                for (size_t frame = 0; frame < end + 3; ++frame)
                {
                    for (unsigned ch = 0; ch < channels; ++ch)
                    {
                        bool zero_base = frame >= 1 && frame < 514;
                        short expected = frame < end ? (ch ? -30000 : 30000) : (ch ? 30000 : -30000);
                        if (zero_base)
                        {
                            expected = frame < end ? (ch ? -32768 : 32767) : 0;
                        }
                        if (!(pcm[frame * channels + ch] == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                if (!(!active(mixer)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Samples saved = pcm;
                if (!(fvm_swf_sound_mixer_add(mixer, end + 3, pcm.begin(), pcm.size(), &written) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!std::memcmp(saved.begin(), pcm.begin(), pcm.size() * sizeof(short))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (size_t i = 0; i < pcm.size(); ++i)
                {
                    pcm[i] = 123;
                }
                if (!(fvm_swf_sound_mixer_read(mixer, end + 3, pcm.begin(), pcm.size(), &written) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (size_t i = 0; i < pcm.size(); ++i)
                {
                    if (!(!pcm[i]))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_swf_sound_mixer_close(mixer);
            }
        }
    }
    short pcm = 123;
    size_t written = 91;
    if (!(fvm_swf_sound_mixer_add(0, 1, &pcm, 1, &written) == FVM_INVALID_ARGUMENT && !written && pcm == 123))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
