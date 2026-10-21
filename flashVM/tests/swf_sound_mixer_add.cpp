/* Base PCM participates in the single clipping pass, across native geometry and chunk boundaries.
 */
#include "swf_sound_conversion_fixture.hpp"
#include <cstring>
using namespace conversion_fixture;
int main()
{
    for (unsigned r = 0; r < 4; ++r)
        for (unsigned channels = 1; channels <= 2; ++channels)
            for (unsigned converted = 0; converted < 2; ++converted)
            {
                FVM_Config cfg;
                FVM_Runtime *rt = 0;
                FVM_Context *c = 0;
                assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
                       fvm_context_create(rt, &c) == FVM_OK);
                Samples source;
                for (unsigned i = 0; i < 600; ++i)
                    for (unsigned ch = 0; ch < channels; ++ch)
                        source.push_back(ch ? -30000 : 30000);
                Bytes swf = movie(source, r, channels);
                assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
                unsigned rate = converted ? 48000 : rates[r];
                FVM_SWFSoundMixer *mixer = 0;
                if (converted)
                    mixer = create(rate, channels);
                else
                    assert(fvm_swf_sound_mixer_create(rate, channels, 2, &mixer) == FVM_OK);
                assert(fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK &&
                       fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK);
                fvm_context_destroy(c);
                fvm_runtime_destroy(rt);
                size_t end = duration(source, rates[r], channels, rate);
                Samples pcm;
                pcm.resize((end + 3) * channels);
                for (size_t i = 0; i < pcm.size(); ++i)
                    pcm[i] = i % channels ? 30000 : -30000;
                size_t written = 91;
                assert(fvm_swf_sound_mixer_add(mixer, end + 3, pcm.begin(), pcm.size() - 1,
                                               &written) == FVM_INVALID_ARGUMENT &&
                       !written && active(mixer) == 2 && pcm[0] == -30000);
                assert(fvm_swf_sound_mixer_add(mixer, (size_t)-1, pcm.begin(), 1, &written) ==
                           FVM_INVALID_ARGUMENT &&
                       !written && active(mixer) == 2);
                assert(fvm_swf_sound_mixer_add(mixer, 0, 0, 0, &written) == FVM_OK && !written);
                assert(fvm_swf_sound_mixer_add(mixer, 1, 0, 1, &written) == FVM_INVALID_ARGUMENT);
                assert(fvm_swf_sound_mixer_add(mixer, 1, pcm.begin(), pcm.size(), 0) ==
                       FVM_INVALID_ARGUMENT);
                size_t at = 0;
                while (at < end + 3)
                {
                    size_t frames = at ? 513 : 1;
                    if (frames > end + 3 - at)
                        frames = end + 3 - at;
                    FVM_Result result =
                        at == 1
                            ? fvm_swf_sound_mixer_read(mixer, frames, pcm.begin() + at * channels,
                                                       pcm.size() - at * channels, &written)
                            : fvm_swf_sound_mixer_add(mixer, frames, pcm.begin() + at * channels,
                                                      pcm.size() - at * channels, &written);
                    assert(result == FVM_OK && written == frames * channels);
                    at += frames;
                }
                for (size_t frame = 0; frame < end + 3; ++frame)
                    for (unsigned ch = 0; ch < channels; ++ch)
                    {
                        bool zero_base = frame >= 1 && frame < 514;
                        short expected =
                            frame < end ? (ch ? -30000 : 30000) : (ch ? 30000 : -30000);
                        if (zero_base)
                            expected = frame < end ? (ch ? -32768 : 32767) : 0;
                        assert(pcm[frame * channels + ch] == expected);
                    }
                assert(!active(mixer));
                Samples saved = pcm;
                assert(fvm_swf_sound_mixer_add(mixer, end + 3, pcm.begin(), pcm.size(), &written) ==
                       FVM_OK);
                assert(!std::memcmp(saved.begin(), pcm.begin(), pcm.size() * sizeof(short)));
                for (size_t i = 0; i < pcm.size(); ++i)
                    pcm[i] = 123;
                assert(fvm_swf_sound_mixer_read(mixer, end + 3, pcm.begin(), pcm.size(),
                                                &written) == FVM_OK);
                for (size_t i = 0; i < pcm.size(); ++i)
                    assert(!pcm[i]);
                fvm_swf_sound_mixer_close(mixer);
            }
    short pcm = 123;
    size_t written = 91;
    assert(fvm_swf_sound_mixer_add(0, 1, &pcm, 1, &written) == FVM_INVALID_ARGUMENT && !written &&
           pcm == 123);
    return 0;
}
