/* Release horizon includes its zero endpoint and never exceeds source exhaustion. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_conversion_fixture.hpp"

using namespace conversion_fixture;


int main()
{
    for (unsigned converted = 0; converted < 2; ++converted)
    {
        for (unsigned length = 5; length <= 1000; length += 995)
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
            Samples samples;
            for (unsigned i = 0; i < length; ++i)
            {
                samples.push_back(10000);
            }
            Bytes swf = movie(samples, 3, 1);
            FVM_SWFSoundVoice *v = 0;
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
                  fvm_swf_sound_voice_open(c, 0, &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned rate = converted ? 48000 : 44100;
            FVM_SWFSoundMixerConfig geometry = {
                sizeof(geometry), rate, 1, 1, FVM_SWF_SOUND_MIXER_CONVERT | FVM_SWF_SOUND_MIXER_DECLICK};
            FVM_SWFSoundMixer *mixer = 0;
            if (!(fvm_swf_sound_mixer_create_configured(&geometry, &mixer) == FVM_OK &&
                  fvm_swf_sound_mixer_submit_prepared(mixer, &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
            fvm_runtime_destroy(rt);
            short pcm;
            size_t written = 0, n = 0;
            if (!(fvm_swf_sound_mixer_read(mixer, 1, &pcm, 1, &written) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_mixer_stop_id(mixer, 7) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            size_t source = (size_t)std::ceil((double)length * rate / 44100) - 1;
            size_t release = rate / 200;
            size_t expected = source < release ? source : release;
            for (size_t i = 0; i < expected; ++i)
            {
                if (!(fvm_swf_sound_mixer_active_frames(mixer, 65536, &n) == FVM_OK && n == expected - i))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_mixer_read(mixer, 1, &pcm, 1, &written) == FVM_OK && written == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_swf_sound_mixer_active_frames(mixer, 65536, &n) == FVM_OK && !n && !active(mixer)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (source >= release)
            {
                if (!(!pcm))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_swf_sound_mixer_close(mixer);
        }
    }
    return 0;
}
